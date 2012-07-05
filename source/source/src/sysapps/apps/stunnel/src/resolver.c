/*
 *   stunnel       Universal SSL tunnel
 *   Copyright (c) 1998-2005 Michal Trojnara <Michal.Trojnara@mirt.net>
 *                 All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   In addition, as a special exception, Michal Trojnara gives
 *   permission to link the code of this program with the OpenSSL
 *   library (or with modified versions of OpenSSL that use the same
 *   license as OpenSSL), and distribute linked combinations including
 *   the two.  You must obey the GNU General Public License in all
 *   respects for all of the code used other than OpenSSL.  If you modify
 *   this file, you may extend this exception to your version of the
 *   file, but you are not obligated to do so.  If you do not wish to
 *   do so, delete this exception statement from your version.
 */

#include "common.h"
#include "prototypes.h"

/**************************************** Resolver functions */

#ifndef HAVE_GETADDRINFO

#ifndef EAI_MEMORY
#define EAI_MEMORY 1
#endif
#ifndef EAI_NONAME
#define EAI_NONAME 2
#endif

#ifdef USE_WIN32
/* rename some locally shadowed declarations */
#define getaddrinfo     local_getaddrinfo
#define freeaddrinfo    local_freeaddrinfo
#define getnameinfo     local_getnameinfo
#else /* defined(USE_WIN32) */
struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    int ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};
#endif /* defined(USE_WIN32) */

static int getaddrinfo(const char *, const char *,
    const struct addrinfo *, struct addrinfo **);
static int alloc_addresses(struct hostent *, const struct addrinfo *,
    u_short port, struct addrinfo **, struct addrinfo **);
static void freeaddrinfo(struct addrinfo *);

#endif /* !defined HAVE_GETADDRINFO */

static const char *s_gai_strerror(int);

#ifndef HAVE_GETNAMEINFO
#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST	2
#endif
#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV	8
#endif
static int getnameinfo(const struct sockaddr *, int,
    char *, int , char *, int , int );
#endif

int name2addrlist(SOCKADDR_LIST *addr_list, char *name, char *default_host) {
    char tmp[STRLEN], *hostname, *portname;

    /* set hostname and portname */
    safecopy(tmp, name);
    portname=strrchr(tmp, ':');
    if(portname) {
        hostname=tmp;
        *portname++='\0';
    } else { /* no ':' - use default host IP */
        hostname=default_host;
        portname=tmp;
    }

    /* fill addr_list structure */
    return hostport2addrlist(addr_list, hostname, portname);
}

int hostport2addrlist(SOCKADDR_LIST *addr_list,
        char *hostname, char *portname) {
    struct addrinfo hints, *res=NULL, *cur;
    int err;

    addr_list->cur=0; /* initialize round-robin counter */

    memset(&hints, 0, sizeof(hints));
#if defined(USE_IPv6) || defined(USE_WIN32)
    hints.ai_family=PF_UNSPEC;
#else
    hints.ai_family=PF_INET;
#endif
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;

    err=getaddrinfo(hostname, portname, &hints, &res);
    if(err) {
        s_log(LOG_ERR, "Error resolving '%s': %s",
            hostname, s_gai_strerror(err));
        if(res)
            freeaddrinfo(res);
        return 0; /* Error */
    }
    /* copy the list of addresses */
    cur=res;
    while(cur && addr_list->num<MAX_HOSTS) {
        if(cur->ai_addrlen>sizeof(SOCKADDR_UNION)) {
            s_log(LOG_ERR, "INTERNAL ERROR: ai_addrlen value too big");
            freeaddrinfo(res);
            return 0; /* no results */
        }
        memcpy(&addr_list->addr[addr_list->num],
            cur->ai_addr, cur->ai_addrlen);
        cur=cur->ai_next;
        addr_list->num++;
    }
    freeaddrinfo(res);
    return addr_list->num; /* ok - return the number of addresses */
}

char *s_ntop(char *text, SOCKADDR_UNION *addr) {
    char host[IPLEN-6], port[6];

    if(getnameinfo(&addr->sa, addr_len(*addr),
            host, IPLEN-6, port, 6, NI_NUMERICHOST|NI_NUMERICSERV)) {
        sockerror("getnameinfo");
        strcpy(text, "unresolvable IP");
        return text;
    }
    strcpy(text, host);
    strcat(text, ":");
    strcat(text, port);
    return text;
}

/**************************************** My getaddrinfo() and getnameinfo() */
/* implementations are limited to functionality needed by stunnel */

#ifndef HAVE_GETADDRINFO
static int getaddrinfo(const char *node, const char *service,
        const struct addrinfo *hints, struct addrinfo **res) {
    struct hostent *h;
    struct servent *p;
    u_short port;
    struct addrinfo *ai;
    int retval;

#ifdef USE_WIN32
    if(s_getaddrinfo)
        return s_getaddrinfo(node, service, hints, res);
#endif
    /* decode service name */
    port=htons((u_short)atoi(service));
    if(!port) { /* zero is an illegal value for port number */
        p=getservbyname(service, "tcp");
        if(!p)
            return EAI_NONAME;
        port=p->s_port;
    }

    /* allocate addrlist structure */
    ai=calloc(1, sizeof(struct addrinfo));
    if(!ai)
        return EAI_MEMORY;
    if(hints)
        memcpy(ai, hints, sizeof(struct addrinfo));

    /* try to decode numerical address */
#if defined(USE_IPv6) && !defined(USE_WIN32)
    ai->ai_family=AF_INET6;
    ai->ai_addrlen=sizeof(struct sockaddr_in6);
    ai->ai_addr=calloc(1, ai->ai_addrlen);
    if(!ai->ai_addr) {
        free(ai);
        return EAI_MEMORY;
    }
    ai->ai_addr->sa_family=AF_INET6;
    if(inet_pton(AF_INET6, node,
            &((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr)>0) {
#else
    ai->ai_family=AF_INET;
    ai->ai_addrlen=sizeof(struct sockaddr_in);
    ai->ai_addr=calloc(1, ai->ai_addrlen);
    if(!ai->ai_addr) {
        free(ai);
        return EAI_MEMORY;
    }
    ai->ai_addr->sa_family=AF_INET;
    ((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr=inet_addr(node);
    if(((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr+1) {
    /* (signed)((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr!=-1 */
#endif
        ((struct sockaddr_in *)ai->ai_addr)->sin_port=port;
        *res=ai;
        return 0; /* numerical address resolved */
    }
    free(ai->ai_addr);
    free(ai);

    /* not numerical: need to call resolver library */
    *res=NULL;
    ai=NULL;
    enter_critical_section(CRIT_INET);
#if defined(USE_IPv6) && !defined(USE_WIN32)
    h=gethostbyname2(node, AF_INET6);
    if(h) /* some IPv6 addresses found */
        alloc_addresses(h, hints, port, res, &ai); /* ignore the error */
#endif
    h=gethostbyname(node); /* get list of addresses */
    if(h)
        retval=ai ?
            alloc_addresses(h, hints, port, &ai->ai_next, &ai) :
            alloc_addresses(h, hints, port, res, &ai);
    else if(!*res)
        retval=EAI_NONAME; /* no results */
    else
        retval=0;
#ifdef HAVE_ENDHOSTENT
    endhostent();
#endif
    leave_critical_section(CRIT_INET);
    if(retval) { /* error: free allocated memory */
        freeaddrinfo(*res);
        *res=NULL;
    }
    return retval;
}

static int alloc_addresses(struct hostent *h, const struct addrinfo *hints,
        u_short port, struct addrinfo **head, struct addrinfo **tail) {
    int i;
    struct addrinfo *ai;

    /* copy addresses */
    for(i=0; h->h_addr_list[i]; i++) {
        ai=calloc(1, sizeof(struct addrinfo));
        if(!ai)
            return EAI_MEMORY;
        if(hints)
            memcpy(ai, hints, sizeof(struct addrinfo));
        ai->ai_next=NULL; /* just in case */
        if(*tail) { /* list not empty: add a node */
            (*tail)->ai_next=ai;
            *tail=ai;
        } else { /* list empty: create it */
            *head=ai;
            *tail=ai;
        }
        ai->ai_family=h->h_addrtype;
#if defined(USE_IPv6)
        if(h->h_addrtype==AF_INET6) {
            ai->ai_addrlen=sizeof(struct sockaddr_in6);
            ai->ai_addr=calloc(1, ai->ai_addrlen);
            if(!ai->ai_addr)
                return EAI_MEMORY;
            memcpy(&((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr,
                h->h_addr_list[i], h->h_length);
        } else
#endif
        {
            ai->ai_addrlen=sizeof(struct sockaddr_in);
            ai->ai_addr=calloc(1, ai->ai_addrlen);
            if(!ai->ai_addr)
                return EAI_MEMORY;
            memcpy(&((struct sockaddr_in *)ai->ai_addr)->sin_addr,
                h->h_addr_list[i], h->h_length);
        }
        ai->ai_addr->sa_family=h->h_addrtype;
        /* offsets of sin_port and sin6_port should be the same */
        ((struct sockaddr_in *)ai->ai_addr)->sin_port=port;
    }
    return 0; /* success */
}

static void freeaddrinfo(struct addrinfo *current) {
    struct addrinfo *next;

#ifdef USE_WIN32
    if(s_freeaddrinfo)
        return s_freeaddrinfo(current);
#endif
    while(current) {
        if(current->ai_addr)
            free(current->ai_addr);
        if(current->ai_canonname)
            free(current->ai_canonname);
        next=current->ai_next;
        free(current);
        current=next;
    }
}

#endif /* !defined HAVE_GETADDRINFO */

/* Due to a problem with Mingw32 I decided to define my own gai_strerror() */
static const char *s_gai_strerror(int err) {
    switch(err) {
#ifdef EAI_BADFLAGS
        case EAI_BADFLAGS:
            return "Invalid value for ai_flags (EAI_BADFLAGS)";
#endif
        case EAI_NONAME:
            return "Neither nodename nor servname known (EAI_NONAME)";
#ifdef EAI_AGAIN
        case EAI_AGAIN:
            return "Temporary failure in name resolution (EAI_AGAIN)";
#endif
#ifdef EAI_FAIL
        case EAI_FAIL:
            return "Non-recoverable failure in name resolution (EAI_FAIL)";
#endif
#ifdef EAI_NODATA
#if EAI_NODATA!=EAI_NONAME
        case EAI_NODATA:
            return "No address associated with nodename (EAI_NODATA)";
#endif /* EAI_NODATA!=EAI_NONAME */
#endif /* defined EAI_NODATA */
#ifdef EAI_FAMILY
        case EAI_FAMILY:
            return "ai_family not supported (EAI_FAMILY)";
#endif
#ifdef EAI_SOCKTYPE
        case EAI_SOCKTYPE:
            return "ai_socktype not supported (EAI_SOCKTYPE)";
#endif
#ifdef EAI_SERVICE
        case EAI_SERVICE:
            return "servname is not supported for ai_socktype (EAI_SERVICE)";
#endif
#ifdef EAI_ADDRFAMILY
        case EAI_ADDRFAMILY:
            return "Address family for nodename not supported (EAI_ADDRFAMILY)";
#endif /* EAI_ADDRFAMILY */
        case EAI_MEMORY:
            return "Memory allocation failure (EAI_MEMORY)";
#ifdef EAI_SYSTEM
        case EAI_SYSTEM:
            return "System error returned in errno (EAI_SYSTEM)";
#endif /* EAI_SYSTEM */
        default:
            return "Unknown error";
    }
}

#ifndef HAVE_GETNAMEINFO
static int getnameinfo(const struct sockaddr *sa, int salen,
    char *host, int hostlen, char *serv, int servlen, int flags) {

#ifdef USE_WIN32
    if(s_getnameinfo)
        return s_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
#endif
    if(host && hostlen) {
#ifdef USE_IPv6
        inet_ntop(sa->sa_family, sa->sa_family==AF_INET6 ? 
                (void *)&((struct sockaddr_in6 *)sa)->sin6_addr :
                (void *)&((struct sockaddr_in *)sa)->sin_addr,
            host, hostlen);
#else /* USE_IPv6 */
        enter_critical_section(CRIT_INET); /* inet_ntoa is not mt-safe */
        strncpy(host, inet_ntoa(((struct sockaddr_in *)sa)->sin_addr),
            hostlen);
        leave_critical_section(CRIT_INET);
        host[hostlen-1]='\0';
#endif /* USE_IPv6 */
    }
    if(serv && servlen)
        sprintf(serv, "%u", ntohs(((struct sockaddr_in *)sa)->sin_port));
    /* sin_port is in the same place both in sockaddr_in and sockaddr_in6 */
    /* ignore servlen since it's long enough in stunnel code */
    return 0;
}
#endif

/**************************************** Some optional debugging stuff */

#if 0
void debug_sockaddr_union(SOCKADDR_UNION *addr) {
    int i;
    char t[3*sizeof(SOCKADDR_UNION)];
    const char hex[16]="0123456789abcdef";
    
    for(i=0; i<sizeof(SOCKADDR_UNION); i++) {
        t[3*i]=hex[((u8 *)addr)[i]/16];
        t[3*i+1]=hex[((u8 *)addr)[i]%16];
        t[3*i+2]=' ';
    }
    t[3*sizeof(SOCKADDR_UNION)-1]='\0';
    s_log(LOG_DEBUG, "%s\n", t);
}
#endif

/* End of resolver.c */
