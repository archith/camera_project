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

#define CONFLINELEN (16*1024)

static int parse_debug_level(char *);
static int parse_ssl_option(char *);
static int print_socket_options(void);
static void print_option(char *, int, OPT_UNION *);
static int parse_socket_option(char *);
static char *section_validate(LOCAL_OPTIONS *);
static char *stralloc(char *);
#ifndef USE_WIN32
static char **argalloc(char *);
#endif

GLOBAL_OPTIONS options;
LOCAL_OPTIONS local_options;

typedef enum {
    CMD_INIT, /* initialize */
    CMD_EXEC,
    CMD_DEFAULT,
    CMD_HELP
} CMD;

static char *option_not_found=
    "Specified option name is not valid here";

static char *global_options(CMD cmd, char *opt, char *arg) {

    if(cmd==CMD_DEFAULT || cmd==CMD_HELP) {
        log_raw("Global options");
    }

    /* CApath */
    switch(cmd) {
    case CMD_INIT:
#if 0
        options.ca_dir=(char *)X509_get_default_cert_dir();
#endif
        options.ca_dir=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "CApath"))
            break;
        if(arg[0]) /* not empty */
            options.ca_dir=stralloc(arg);
        else
            options.ca_dir=NULL;
        return NULL; /* OK */
    case CMD_DEFAULT:
#if 0
        log_raw("%-15s = %s", "CApath",
            options.ca_dir ? options.ca_dir : "(none)");
#endif
        break;
    case CMD_HELP:
        log_raw("%-15s = CA certificate directory for 'verify' option",
            "CApath");
        break;
    }

    /* CAfile */
    switch(cmd) {
    case CMD_INIT:
#if 0
        options.ca_file=(char *)X509_get_default_certfile();
#endif
        options.ca_file=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "CAfile"))
            break;
        if(arg[0]) /* not empty */
            options.ca_file=stralloc(arg);
        else
            options.ca_file=NULL;
        return NULL; /* OK */
    case CMD_DEFAULT:
#if 0
        log_raw("%-15s = %s", "CAfile",
            options.ca_file ? options.ca_file : "(none)");
#endif
        break;
    case CMD_HELP:
        log_raw("%-15s = CA certificate file for 'verify' option",
            "CAfile");
        break;
    }

    /* cert */
    switch(cmd) {
    case CMD_INIT:
#ifdef CONFDIR
        options.cert=CONFDIR "/stunnel.pem";
#else
        options.cert="stunnel.pem";
#endif
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "cert"))
            break;
        options.cert=stralloc(arg);
        options.option.cert=1;
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %s", "cert", options.cert);
        break;
    case CMD_HELP:
        log_raw("%-15s = certificate chain", "cert");
        break;
    }

    /* chroot */
#ifdef HAVE_CHROOT
    switch(cmd) {
    case CMD_INIT:
        options.chroot_dir=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "chroot"))
            break;
        options.chroot_dir=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = directory to chroot stunnel process", "chroot");
        break;
    }
#endif /* HAVE_CHROOT */

    /* ciphers */
    switch(cmd) {
    case CMD_INIT:
        options.cipher_list=SSL_DEFAULT_CIPHER_LIST;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "ciphers"))
            break;
        options.cipher_list=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %s", "ciphers", SSL_DEFAULT_CIPHER_LIST);
        break;
    case CMD_HELP:
        log_raw("%-15s = list of permitted SSL ciphers", "ciphers");
        break;
    }

    /* client */
    switch(cmd) {
    case CMD_INIT:
        options.option.client=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "client"))
            break;
        if(!strcasecmp(arg, "yes"))
            options.option.client=1;
        else if(!strcasecmp(arg, "no"))
            options.option.client=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no client mode (remote service uses SSL)",
            "client");
        break;
    }

    /* compression */
    switch(cmd) {
    case CMD_INIT:
        options.compression=COMP_NONE;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "compression"))
            break;
        if(!strcasecmp(arg, "zlib"))
            options.compression=COMP_ZLIB;
        else if(!strcasecmp(arg, "rle"))
            options.compression=COMP_RLE;
        else
            return "compression type should be either 'zlib' or 'rle'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = zlib|rle compression type",
            "compression");
        break;
    }

    /* CRLpath */
    switch(cmd) {
    case CMD_INIT:
        options.crl_dir=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "CRLpath"))
            break;
        if(arg[0]) /* not empty */
            options.crl_dir=stralloc(arg);
        else
            options.crl_dir=NULL;
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = CRL directory", "CRLpath");
        break;
    }

    /* CRLfile */
    switch(cmd) {
    case CMD_INIT:
        options.crl_file=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "CRLfile"))
            break;
        if(arg[0]) /* not empty */
            options.crl_file=stralloc(arg);
        else
            options.crl_file=NULL;
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = CRL file", "CRLfile");
        break;
    }

    /* debug */
    switch(cmd) {
    case CMD_INIT:
        options.debug_level=5;
#if !defined (USE_WIN32) && !defined (__vms)
        options.facility=LOG_DAEMON;
#endif
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "debug"))
            break;
        if(!parse_debug_level(arg))
            return "Illegal debug argument";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %d", "debug", options.debug_level);
        break;
    case CMD_HELP:
        log_raw("%-15s = [facility].level (e.g. daemon.info)", "debug");
        break;
    }

    /* EGD is only supported when compiled with OpenSSL 0.9.5a or later */
#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
    switch(cmd) {
    case CMD_INIT:
        options.egd_sock=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "EGD"))
            break;
        options.egd_sock=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
#ifdef EGD_SOCKET
        log_raw("%-15s = %s", "EGD", EGD_SOCKET);
#endif
        break;
    case CMD_HELP:
        log_raw("%-15s = path to Entropy Gathering Daemon socket", "EGD");
        break;
    }
#endif /* OpenSSL 0.9.5a */

#if (SSLEAY_VERSION_NUMBER >= 0x00907000L) && defined(HAVE_OSSL_ENGINE_H)
    /* engine */
    switch(cmd) {
    case CMD_INIT:
        options.engine=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "engine"))
            break;
        options.engine=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = auto|engine_id",
            "engine");
        break;
    }
#endif

    /* foreground */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        options.option.syslog=0;
        options.option.foreground=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "foreground"))
            break;
        if(!strcasecmp(arg, "yes"))
            options.option.foreground=1;
        else if(!strcasecmp(arg, "no"))
            options.option.foreground=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no foreground mode (don't fork, log to stderr)",
            "foreground");
        break;
    }
#endif

    /* key */
    switch(cmd) {
    case CMD_INIT:
        options.key=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "key"))
            break;
        options.key=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %s", "key", options.cert); /* set in stunnel.c */
        break;
    case CMD_HELP:
        log_raw("%-15s = certificate private key", "key");
        break;
    }

    /* options */
    switch(cmd) {
    case CMD_INIT:
        options.ssl_options=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "options"))
            break;
        if(!parse_ssl_option(arg))
            return "Illegal SSL option";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = SSL option", "options");
        log_raw("%18sset an SSL option", "");
        break;
    }

    /* output */
    switch(cmd) {
    case CMD_INIT:
        options.output_file=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "output"))
            break;
        options.output_file=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = file to append log messages", "output");
        break;
    }

    /* pid */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        options.pidfile=PIDFILE;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "pid"))
            break;
        if(arg[0]) /* is argument not empty? */
            options.pidfile=stralloc(arg);
        else
            options.pidfile=NULL; /* empty -> do not create a pid file */
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %s", "pid", PIDFILE);
        break;
    case CMD_HELP:
        log_raw("%-15s = pid file (empty to disable creating)", "pid");
        break;
    }
#endif

    /* RNDbytes */
    switch(cmd) {
    case CMD_INIT:
        options.random_bytes=RANDOM_BYTES;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "RNDbytes"))
            break;
        options.random_bytes=atoi(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %d", "RNDbytes", RANDOM_BYTES);
        break;
    case CMD_HELP:
        log_raw("%-15s = bytes to read from random seed files", "RNDbytes");
        break;
    }

    /* RNDfile */
    switch(cmd) {
    case CMD_INIT:
        options.rand_file=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "RNDfile"))
            break;
        options.rand_file=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
#ifdef RANDOM_FILE
        log_raw("%-15s = %s", "RNDfile", RANDOM_FILE);
#endif
        break;
    case CMD_HELP:
        log_raw("%-15s = path to file with random seed data", "RNDfile");
        break;
    }

    /* RNDoverwrite */
    switch(cmd) {
    case CMD_INIT:
        options.option.rand_write=1;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "RNDoverwrite"))
            break;
        if(!strcasecmp(arg, "yes"))
            options.option.rand_write=1;
        else if(!strcasecmp(arg, "no"))
            options.option.rand_write=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = yes", "RNDoverwrite");
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no overwrite seed datafiles with new random data",
            "RNDoverwrite");
        break;
    }

    /* service */
    switch(cmd) {
    case CMD_INIT:
        local_options.servname=stralloc("stunnel");
#ifdef USE_WIN32
        options.win32_service="stunnel";
        options.win32_name="stunnel " VERSION " on Win32";
#endif
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "service"))
            break;
        local_options.servname=stralloc(arg);
#ifdef USE_WIN32
        options.win32_service=stralloc(arg);
        {
            char tmpstr[STRLEN];

            safecopy(tmpstr, "stunnel " VERSION " on Win32 (");
            safeconcat(tmpstr, arg);
            safeconcat(tmpstr, ")");
            options.win32_name=stralloc(tmpstr);
        }
#endif
        return NULL; /* OK */
    case CMD_DEFAULT:
#ifdef USE_WIN32
        log_raw("%-15s = %s", "service", options.win32_service);
#endif
        break;
    case CMD_HELP:
        log_raw("%-15s = service name", "service");
        break;
    }

    /* session */
    switch(cmd) {
    case CMD_INIT:
        options.session_timeout=300;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "session"))
            break;
        if(atoi(arg)>0)
            options.session_timeout=atoi(arg);
        else
            return "Illegal session timeout";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %ld seconds", "session", options.session_timeout);
        break;
    case CMD_HELP:
        log_raw("%-15s = session cache timeout (in seconds)", "session");
        break;
    }

#ifndef USE_WIN32
    /* setgid */
    switch(cmd) {
    case CMD_INIT:
        options.setgid_group=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "setgid"))
            break;
        options.setgid_group=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = groupname for setgid()", "setgid");
        break;
    }
#endif

#ifndef USE_WIN32
    /* setuid */
    switch(cmd) {
    case CMD_INIT:
        options.setuid_user=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "setuid"))
            break;
        options.setuid_user=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = username for setuid()", "setuid");
        break;
    }
#endif

    /* socket */
    switch(cmd) {
    case CMD_INIT:
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "socket"))
            break;
        if(!parse_socket_option(arg))
            return "Illegal socket option";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = a|l|r:option=value[:value]", "socket");
        log_raw("%18sset an option on accept/local/remote socket", "");
        break;
    }

    /* taskbar */
#ifdef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        options.option.taskbar=1;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "taskbar"))
            break;
        if(!strcasecmp(arg, "yes"))
            options.option.taskbar=1;
        else if(!strcasecmp(arg, "no"))
            options.option.taskbar=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = yes", "taskbar");
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no enable the taskbar icon", "taskbar");
        break;
    }
#endif

    /* verify */
    switch(cmd) {
    case CMD_INIT:
        options.verify_level=-1;
        options.verify_use_only_my=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "verify"))
            break;
        options.verify_level=SSL_VERIFY_NONE;
        switch(atoi(arg)) {
        case 3:
            options.verify_use_only_my=1;
        case 2:
            options.verify_level|=SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        case 1:
            options.verify_level|=SSL_VERIFY_PEER;
        case 0:
            return NULL; /* OK */
        default:
            return "Bad verify level";
        }
    case CMD_DEFAULT:
        log_raw("%-15s = none", "verify");
        break;
    case CMD_HELP:
        log_raw("%-15s = level of peer certificate verification", "verify");
        log_raw("%18slevel 1 - verify peer certificate if present", "");
        log_raw("%18slevel 2 - require valid peer certificate always", "");
        log_raw("%18slevel 3 - verify peer with locally installed certificate",
        "");
        break;
    }

    if(cmd==CMD_EXEC)
        return option_not_found;
    return NULL; /* OK */
}

static char *service_options(CMD cmd, LOCAL_OPTIONS *section,
        char *opt, char *arg) {

    if(cmd==CMD_DEFAULT || cmd==CMD_HELP) {
        log_raw(" ");
        log_raw("Service-level options");
    }

    /* accept */
    switch(cmd) {
    case CMD_INIT:
        section->option.accept=0;
        memset(&section->local_addr, 0, sizeof(SOCKADDR_LIST));
        section->local_addr.addr[0].in.sin_family=AF_INET;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "accept"))
            break;
        section->option.accept=1;
        if(!name2addrlist(&section->local_addr, arg, DEFAULT_ANY))
            exit(2);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = [host:]port accept connections on specified host:port",
            "accept");
        break;
    }

    /* connect */
    switch(cmd) {
    case CMD_INIT:
        section->option.remote=0;
        section->remote_address=NULL;
        section->remote_addr.num=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "connect"))
            break;
        section->option.remote=1;
        section->remote_address=stralloc(arg);
        if(!section->option.delayed_lookup &&
                !name2addrlist(&section->remote_addr, arg, DEFAULT_LOOPBACK)) {
            log_raw("Cannot resolve '%s' - delaying DNS lookup", arg);
            section->option.delayed_lookup=1;
        }
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = [host:]port connect remote host:port",
            "connect");
        break;
    }

    /* delay */
    switch(cmd) {
    case CMD_INIT:
        section->option.delayed_lookup=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "delay"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.delayed_lookup=1;
        else if(!strcasecmp(arg, "no"))
            section->option.delayed_lookup=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no delay DNS lookup for 'connect' option",
            "delay");
        break;
    }

    /* exec */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        section->option.program=0;
        section->execname=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "exec"))
            break;
        section->option.program=1;
        section->execname=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = file execute local inetd-type program",
            "exec");
        break;
    }
#endif

    /* execargs */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        section->execargs=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "execargs"))
            break;
        section->execargs=argalloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = arguments for 'exec' (including $0)",
            "execargs");
        break;
    }
#endif

    /* ident */
    switch(cmd) {
    case CMD_INIT:
        section->username=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "ident"))
            break;
        section->username=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = username for IDENT (RFC 1413) checking", "ident");
        break;
    }

    /* local */
    switch(cmd) {
    case CMD_INIT:
        memset(&section->source_addr, 0, sizeof(SOCKADDR_LIST));
        section->source_addr.addr[0].in.sin_family=AF_INET;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "local"))
            break;
        if(!hostport2addrlist(&section->source_addr, arg, "0"))
            exit(2);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = IP address to be used as source for remote"
            " connections", "local");
        break;
    }

    /* protocol */
    switch(cmd) {
    case CMD_INIT:
        section->protocol=NULL;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "protocol"))
            break;
        section->protocol=stralloc(arg);
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = protocol to negotiate before SSL initialization",
            "protocol");
        log_raw("%18scurrently supported: cifs, nntp, pop3, smtp", "");
        break;
    }

    /* pty */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        section->option.pty=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "pty"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.pty=1;
        else if(!strcasecmp(arg, "no"))
            section->option.pty=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no allocate pseudo terminal for 'exec' option",
            "pty");
        break;
    }
#endif

    /* TIMEOUTbusy */
    switch(cmd) {
    case CMD_INIT:
        section->timeout_busy=300; /* 5 minutes */
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "TIMEOUTbusy"))
            break;
        if(atoi(arg)>0)
            section->timeout_busy=atoi(arg);
        else
            return "Illegal busy timeout";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %d seconds", "TIMEOUTbusy", section->timeout_busy);
        break;
    case CMD_HELP:
        log_raw("%-15s = seconds to wait for expected data", "TIMEOUTbusy");
        break;
    }

    /* TIMEOUTclose */
    switch(cmd) {
    case CMD_INIT:
        section->timeout_close=60; /* 1 minute */
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "TIMEOUTclose"))
            break;
        if(atoi(arg)>0 || !strcmp(arg, "0"))
            section->timeout_close=atoi(arg);
        else
            return "Illegal close timeout";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %d seconds", "TIMEOUTclose", section->timeout_close);
        break;
    case CMD_HELP:
        log_raw("%-15s = seconds to wait for close_notify"
            " (set to 0 for buggy MSIE)", "TIMEOUTclose");
        break;
    }

    /* TIMEOUTconnect */
    switch(cmd) {
    case CMD_INIT:
        section->timeout_connect=10; /* 10 seconds */
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "TIMEOUTconnect"))
            break;
        if(atoi(arg)>0 || !strcmp(arg, "0"))
            section->timeout_connect=atoi(arg);
        else
            return "Illegal connect timeout";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %d seconds", "TIMEOUTconnect",
            section->timeout_connect);
        break;
    case CMD_HELP:
        log_raw("%-15s = seconds to connect remote host", "TIMEOUTconnect");
        break;
    }

    /* TIMEOUTidle */
    switch(cmd) {
    case CMD_INIT:
        section->timeout_idle=43200; /* 12 hours */
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "TIMEOUTidle"))
            break;
        if(atoi(arg)>0)
            section->timeout_idle=atoi(arg);
        else
            return "Illegal idle timeout";
        return NULL; /* OK */
    case CMD_DEFAULT:
        log_raw("%-15s = %d seconds", "TIMEOUTidle", section->timeout_idle);
        break;
    case CMD_HELP:
        log_raw("%-15s = seconds to keep an idle connection", "TIMEOUTidle");
        break;
    }

    /* transparent */
#ifndef USE_WIN32
    switch(cmd) {
    case CMD_INIT:
        section->option.transparent=0;
        break;
    case CMD_EXEC:
        if(strcasecmp(opt, "transparent"))
            break;
        if(!strcasecmp(arg, "yes"))
            section->option.transparent=1;
        else if(!strcasecmp(arg, "no"))
            section->option.transparent=0;
        else
            return "argument should be either 'yes' or 'no'";
        return NULL; /* OK */
    case CMD_DEFAULT:
        break;
    case CMD_HELP:
        log_raw("%-15s = yes|no transparent proxy mode",
            "transparent");
        break;
    }
#endif

    if(cmd==CMD_EXEC)
        return option_not_found;
    return NULL; /* OK */
}

static void syntax(char *confname) {
    log_raw(" ");
    log_raw("Syntax:");
#ifdef USE_WIN32
    log_raw("stunnel [ [-install | -uninstall] [-quiet] [<filename>] ]"
        " | -help | -version | -sockets");
#else
    log_raw("stunnel [<filename>] | -fd <n> | -help | -version | -sockets");
#endif
    log_raw("    <filename>  - use specified config file instead of %s",
        confname);
#ifdef USE_WIN32
    log_raw("    -install    - install NT service");
    log_raw("    -uninstall  - uninstall NT service");
    log_raw("    -quiet      - don't display a message box on success");
#else
    log_raw("    -fd <n>     - read the config file from a file descriptor");
#endif
    log_raw("    -help       - get config file help");
    log_raw("    -version    - display version and defaults");
    log_raw("    -sockets    - display default socket options");
    exit(1);
}

void parse_config(char *name, char *parameter) {
#ifdef CONFDIR
    char *default_config_file=CONFDIR "/stunnel.conf";
#else
    char *default_config_file="stunnel.conf";
#endif
    FILE *fp, *fp2=NULL;
    char confline[CONFLINELEN], *arg, *opt, *errstr, *filename, *filename2;
    int line_number, i, j, aport=0, conflict=0;
    LOCAL_OPTIONS *section, *new_section, *pre_section;
    
    memset(&options, 0, sizeof(GLOBAL_OPTIONS)); /* reset global options */

    memset(&local_options, 0, sizeof(LOCAL_OPTIONS)); /* reset local options */
    local_options.next=NULL;
    section=&local_options;
    pre_section = section;

    global_options(CMD_INIT, NULL, NULL);
    service_options(CMD_INIT, section, NULL, NULL);
    if(!name)
        name=default_config_file;
    if(!strcasecmp(name, "-help")) {
        global_options(CMD_HELP, NULL, NULL);
        service_options(CMD_HELP, section, NULL, NULL);
        exit(1);
    }
    if(!strcasecmp(name, "-version")) {
        log_raw("%s", stunnel_info());
        log_raw(" ");
        global_options(CMD_DEFAULT, NULL, NULL);
        service_options(CMD_DEFAULT, section, NULL, NULL);
        exit(1);
    }
    if(!strcasecmp(name, "-sockets")) {
        print_socket_options();
        exit(1);
    }
#ifndef USE_WIN32
    if(!strcasecmp(name, "-fd")) {
        if(!parameter) {
            log_raw("No file descriptor specified");
            syntax(default_config_file);
        }
        for(arg=parameter, i=0; *arg; arg++) {
            if(*arg<'0' || *arg>'9') {
                log_raw("Invalid file descriptor %s", parameter);
                syntax(default_config_file);
            }
            i=10*i+*arg-'0';
        }
        fp=fdopen(i, "r");
        if(!fp) {
            log_raw("Invalid file descriptor %s", parameter);
            syntax(default_config_file);
        }
        filename="descriptor";
    } else
#endif
    {
        fp=fopen(name, "r");
        if(!fp) {
#ifdef USE_WIN32
            /* fopen() does not return the error via GetLastError() on Win32 */
            log_raw("Failed to open configuration file %s", name);
#else
            ioerror(name);
#endif
            syntax(default_config_file);
        }
        filename=name;

	/* MODIFIED: second config file */
	if(parameter)
	{
		fp2 = fopen(parameter, "r");
		if(!fp2)
		{
			ioerror(parameter);
			exit(1);
		}
		filename2 = parameter;
	}
    }

   for(j=0; j<2; j++)
   {
	if(j==1)
	{
		if(fp2 != NULL)
			fp = fp2;
		else
			break;
	}

    line_number=0;
    while(fgets(confline, CONFLINELEN, fp)) {
        line_number++;
        opt=confline;
        while(isspace((unsigned char)*opt))
            opt++; /* remove initial whitespaces */
        for(i=strlen(opt)-1; i>=0 && isspace((unsigned char)opt[i]); i--)
            opt[i]='\0'; /* remove trailing whitespaces */
        if(opt[0]=='\0' || opt[0]=='#' || opt[0]==';') /* empty or comment */
            continue;

	if(conflict == 1)
	{
		if(opt[0]!='[')
			continue;
		else
			conflict = 0;
	}

        if(opt[0]=='[' && opt[strlen(opt)-1]==']') { /* new section */
            errstr=section_validate(section);
            if(errstr) {
                log_raw("file %s line %d: %s", filename, line_number, errstr);
                exit(1);
            }
            opt++;
            opt[strlen(opt)-1]='\0';
            new_section=calloc(1, sizeof(LOCAL_OPTIONS));
            if(!new_section) {
                log_raw("Fatal memory allocation error");
                exit(2);
            }
            memcpy(new_section, &local_options, sizeof(LOCAL_OPTIONS));
            new_section->servname=stralloc(opt);
            new_section->session=NULL;
            new_section->next=NULL;
		pre_section = section;
            section->next=new_section;
            section=new_section;
            continue;
        }
        arg=strchr(confline, '=');
        if(!arg) {
            log_raw("file %s line %d: No '=' found", filename, line_number);
            exit(1);
        }
        *arg++='\0'; /* split into option name and argument value */
        for(i=strlen(opt)-1; i>=0 && isspace((unsigned char)opt[i]); i--)
            opt[i]='\0'; /* remove trailing whitespaces */
        while(isspace((unsigned char)*arg))
            arg++; /* remove initial whitespaces */

	if(strcasecmp(opt, "accept")==0)
	{
		if(aport != 0 && aport == atoi(arg))
		{
			conflict = 1;
			free(section->servname);
			free(section);
			section = pre_section;
			section->next = NULL;
			continue;
		}
		aport = atoi(arg);
	}

        errstr=service_options(CMD_EXEC, section, opt, arg);
        if(section==&local_options && errstr==option_not_found)
            errstr=global_options(CMD_EXEC, opt, arg);
        if(errstr) {
            log_raw("file %s line %d: %s", filename, line_number, errstr);
            exit(1);
        }
    }
    errstr=section_validate(section);
    if(errstr) {
        log_raw("file %s line %d: %s", filename, line_number, errstr);
        exit(1);
    }
    fclose(fp);

   }

#if 0
    /* add by yen for alternative https port (accept) and http port (connect) */
    if(!strcasecmp(parameter, "-p")) {
	new_section=calloc(1, sizeof(LOCAL_OPTIONS));
	if(!new_section) {
		log_raw("Fatal memory allocation error");
		exit(2);
	}
	memcpy(new_section, &local_options, sizeof(LOCAL_OPTIONS));
	new_section->servname=stralloc("stunnel");
	new_section->session=NULL;
	new_section->next=NULL;
	section->next=new_section;
	section=new_section;

	section->option.accept = 1;
	section->option.remote = 1;
	if(!name2addrlist(&section->local_addr, aportstr, DEFAULT_ANY))
            exit(2);
	if(!name2addrlist(&section->remote_addr, cportstr, DEFAULT_ANY))
            exit(2);
//	printf("!!!!! address0: %s\n", inet_ntoa(((struct sockaddr_in*)(&(section->local_addr.addr[0].sa)))->sin_addr));
//	printf("!!!!! port0: %u\n", ntohs(((struct sockaddr_in*)(&(section->local_addr.addr[0].sa)))->sin_port));
    }
#endif

    if(!local_options.next) { /* inetd mode */
        if (section->option.accept) {
            log_raw("accept option is not allowed in inetd mode");
            log_raw("remove accept option or define a [section]");
            exit(1);
        }
        if (!section->option.remote && !section->execname) {
            log_raw("inetd mode must define a remote host or an executable");
            exit(1);
        }
    }
    if(!options.option.client)
        options.option.cert=1; /* Server always needs a certificate */
    if(!options.option.foreground)
        options.option.syslog=1;
}

static char *section_validate(LOCAL_OPTIONS *section) {
    if(section==&local_options)
        return NULL; /* No need to validate defaults */
#ifdef USE_WIN32
    if(!section->option.accept || !section->option.remote)
#else
    if((unsigned int)section->option.accept +
            (unsigned int)section->option.program +
            (unsigned int)section->option.remote != 2)
#endif
        return "Each service section must define exactly two endpoints";
    return NULL; /* All tests passed -- continue program execution */
}

static char *stralloc(char *str) { /* Allocate static string */
    char *retval;
    
    retval=calloc(strlen(str)+1, 1);
    if(!retval) {
        log_raw("Fatal memory allocation error");
        exit(2);
    }
    strcpy(retval, str);
    return retval;
}

#ifndef USE_WIN32
static char **argalloc(char *str) { /* Allocate 'exec' argumets */
    int max_arg, i;
    char *ptr, **retval;

    max_arg=strlen(str)/2+1;
    ptr=stralloc(str);
    retval=calloc(max_arg+1, sizeof(char *));
    if(!retval) {
        log_raw("Fatal memory allocation error");
        exit(2);
    }
    i=0;
    while(*ptr && i<max_arg) {
        retval[i++]=ptr;
        while(*ptr && !isspace((unsigned char)*ptr))
            ptr++;
        while(*ptr && isspace((unsigned char)*ptr))
            *ptr++='\0';
    }
    retval[i]=NULL;
    return retval;
}
#endif

/* Parse out the facility/debug level stuff */

typedef struct {
    char *name;
    int value;
} facilitylevel;

static int parse_debug_level(char *arg) {
    char arg_copy[STRLEN];
    char *string;
    facilitylevel *fl;

/* Facilities only make sense on unix */
#if !defined (USE_WIN32) && !defined (__vms)
    facilitylevel facilities[] = {
        {"auth", LOG_AUTH},     {"cron", LOG_CRON},     {"daemon", LOG_DAEMON},
        {"kern", LOG_KERN},     {"lpr", LOG_LPR},       {"mail", LOG_MAIL},
        {"news", LOG_NEWS},     {"syslog", LOG_SYSLOG}, {"user", LOG_USER},
        {"uucp", LOG_UUCP},     {"local0", LOG_LOCAL0}, {"local1", LOG_LOCAL1},
        {"local2", LOG_LOCAL2}, {"local3", LOG_LOCAL3}, {"local4", LOG_LOCAL4},
        {"local5", LOG_LOCAL5}, {"local6", LOG_LOCAL6}, {"local7", LOG_LOCAL7},

        /* Some that are not on all unicies */
#ifdef LOG_AUTHPRIV
        {"authpriv", LOG_AUTHPRIV},
#endif
#ifdef LOG_FTP
        {"ftp", LOG_FTP},
#endif
#ifdef LOG_NTP
        {"ntp", LOG_NTP},
#endif
        {NULL, 0}
    };
#endif /* USE_WIN32, __vms */

    facilitylevel levels[] = {
        {"emerg", LOG_EMERG},     {"alert", LOG_ALERT},
        {"crit", LOG_CRIT},       {"err", LOG_ERR},
        {"warning", LOG_WARNING}, {"notice", LOG_NOTICE},
        {"info", LOG_INFO},       {"debug", LOG_DEBUG},
        {NULL, -1}
    };

    safecopy(arg_copy, arg);
    string = arg_copy;

/* Facilities only make sense on unix */
#if !defined (USE_WIN32) && !defined (__vms)
    if(strchr(string, '.')) { /* We have a facility specified */
        options.facility=-1;
        string=strtok(arg_copy, "."); /* break it up */

        for(fl=facilities; fl->name; fl++) {
            if(!strcasecmp(fl->name, string)) {
                options.facility = fl->value;
                break;
            }
        }
        if(options.facility==-1)
            return 0; /* FAILED */
        string=strtok(NULL, ".");    /* set to the remainder */
    }
#endif /* USE_WIN32, __vms */

    /* Time to check the syslog level */
    if(string && strlen(string)==1 && *string>='0' && *string<='7') {
        options.debug_level=*string-'0';
        return 1; /* OK */
    }
    options.debug_level=8;    /* illegal level */
    for(fl=levels; fl->name; fl++) {
        if(!strcasecmp(fl->name, string)) {
            options.debug_level=fl->value;
            break;
        }
    }
    if (options.debug_level==8)
        return 0; /* FAILED */
    return 1; /* OK */
}

/* Parse SSL options stuff */

static int parse_ssl_option(char *arg) {
    struct {
        char *name;
        long value;
    } ssl_opts[] = {
        {"MICROSOFT_SESS_ID_BUG", SSL_OP_MICROSOFT_SESS_ID_BUG},
        {"NETSCAPE_CHALLENGE_BUG", SSL_OP_NETSCAPE_CHALLENGE_BUG},
        {"NETSCAPE_REUSE_CIPHER_CHANGE_BUG",
            SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG},
        {"SSLREF2_REUSE_CERT_TYPE_BUG", SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG},
        {"MICROSOFT_BIG_SSLV3_BUFFER", SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER},
        {"MSIE_SSLV2_RSA_PADDING", SSL_OP_MSIE_SSLV2_RSA_PADDING},
        {"SSLEAY_080_CLIENT_DH_BUG", SSL_OP_SSLEAY_080_CLIENT_DH_BUG},
        {"TLS_D5_BUG", SSL_OP_TLS_D5_BUG},
        {"TLS_BLOCK_PADDING_BUG", SSL_OP_TLS_BLOCK_PADDING_BUG},
        {"TLS_ROLLBACK_BUG", SSL_OP_TLS_ROLLBACK_BUG},
#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
        {"DONT_INSERT_EMPTY_FRAGMENTS", SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS},
#endif
        {"ALL", SSL_OP_ALL},
        {"SINGLE_DH_USE", SSL_OP_SINGLE_DH_USE},
        {"EPHEMERAL_RSA", SSL_OP_EPHEMERAL_RSA},
        {"NO_SSLv2", SSL_OP_NO_SSLv2},
        {"NO_SSLv3", SSL_OP_NO_SSLv3},
        {"NO_TLSv1", SSL_OP_NO_TLSv1},
        {"PKCS1_CHECK_1", SSL_OP_PKCS1_CHECK_1},
        {"PKCS1_CHECK_2", SSL_OP_PKCS1_CHECK_2},
        {"NETSCAPE_CA_DN_BUG", SSL_OP_NETSCAPE_CA_DN_BUG},
#ifdef SSL_OP_NON_EXPORT_FIRST
        {"NON_EXPORT_FIRST", SSL_OP_NON_EXPORT_FIRST},
#endif
        {"NETSCAPE_DEMO_CIPHER_CHANGE_BUG",
            SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG},
        {NULL, 0}
    }, *option;

    for(option=ssl_opts; option->name; option++)
        if(!strcasecmp(option->name, arg)) {
            options.ssl_options|=option->value;
            return 1; /* OK */
        }
    return 0; /* FAILED */
}

/* Parse out the socket options stuff */

static int on=1;

#define DEF_VALS {NULL, NULL, NULL}
#define DEF_ACCEPT {(void *)&on, NULL, NULL}

SOCK_OPT sock_opts[] = {
    {"SO_DEBUG",        SOL_SOCKET,  SO_DEBUG,        TYPE_FLAG,    DEF_VALS},
    {"SO_DONTROUTE",    SOL_SOCKET,  SO_DONTROUTE,    TYPE_FLAG,    DEF_VALS},
    {"SO_KEEPALIVE",    SOL_SOCKET,  SO_KEEPALIVE,    TYPE_FLAG,    DEF_VALS},
    {"SO_LINGER",       SOL_SOCKET,  SO_LINGER,       TYPE_LINGER,  DEF_VALS},
    {"SO_OOBINLINE",    SOL_SOCKET,  SO_OOBINLINE,    TYPE_FLAG,    DEF_VALS},
    {"SO_RCVBUF",       SOL_SOCKET,  SO_RCVBUF,       TYPE_INT,     DEF_VALS},
    {"SO_SNDBUF",       SOL_SOCKET,  SO_SNDBUF,       TYPE_INT,     DEF_VALS},
#ifdef SO_RCVLOWAT
    {"SO_RCVLOWAT",     SOL_SOCKET,  SO_RCVLOWAT,     TYPE_INT,     DEF_VALS},
#endif
#ifdef SO_SNDLOWAT
    {"SO_SNDLOWAT",     SOL_SOCKET,  SO_SNDLOWAT,     TYPE_INT,     DEF_VALS},
#endif
#ifdef SO_RCVTIMEO
    {"SO_RCVTIMEO",     SOL_SOCKET,  SO_RCVTIMEO,     TYPE_TIMEVAL, DEF_VALS},
#endif
#ifdef SO_SNDTIMEO
    {"SO_SNDTIMEO",     SOL_SOCKET,  SO_SNDTIMEO,     TYPE_TIMEVAL, DEF_VALS},
#endif
    {"SO_REUSEADDR",    SOL_SOCKET,  SO_REUSEADDR,    TYPE_FLAG,    DEF_ACCEPT},
#ifdef SO_BINDTODEVICE
    {"SO_BINDTODEVICE", SOL_SOCKET,  SO_BINDTODEVICE, TYPE_STRING,  DEF_VALS},
#endif
#ifdef IP_TOS
    {"IP_TOS",          IPPROTO_IP,  IP_TOS,          TYPE_INT,     DEF_VALS},
#endif
#ifdef IP_TTL
    {"IP_TTL",          IPPROTO_IP,  IP_TTL,          TYPE_INT,     DEF_VALS},
#endif
#ifdef IP_MAXSEG
    {"TCP_MAXSEG",      IPPROTO_TCP, TCP_MAXSEG,      TYPE_INT,     DEF_VALS},
#endif
    {"TCP_NODELAY",     IPPROTO_TCP, TCP_NODELAY,     TYPE_FLAG,    DEF_VALS},
    {NULL,              0,           0,               TYPE_NONE,    DEF_VALS}
};

static int print_socket_options(void) {
    int fd;
    socklen_t optlen;
    SOCK_OPT *ptr;
    OPT_UNION val;
    char line[STRLEN];

    fd=socket(AF_INET, SOCK_STREAM, 0);

    log_raw("Socket option defaults:");
    log_raw("    %-16s%-10s%-10s%-10s%-10s",
        "Option", "Accept", "Local", "Remote", "OS default");
    for(ptr=sock_opts; ptr->opt_str; ptr++) {
        /* display option name */
        sprintf(line, "    %-16s", ptr->opt_str);
        /* display stunnel default values */
        print_option(line, ptr->opt_type, ptr->opt_val[0]);
        print_option(line, ptr->opt_type, ptr->opt_val[1]);
        print_option(line, ptr->opt_type, ptr->opt_val[2]);
        /* display OS default value */
        optlen=sizeof(val);
        if(getsockopt(fd, ptr->opt_level,
                ptr->opt_name, (void *)&val, &optlen)) {
            if(get_last_socket_error()!=ENOPROTOOPT) {
                log_raw("%s", line); /* dump the name and assigned values */
                sockerror("getsockopt");
                return 0; /* FAILED */
            }
            safeconcat(line, "    --    "); /* write-only value */
        } else
            print_option(line, ptr->opt_type, &val);
        log_raw("%s", line);
    }
    return 1; /* OK */
}

static void print_option(char *line, int type, OPT_UNION *val) {
    char text[STRLEN];

    if(!val) {
        safecopy(text, "    --    ");
    } else {
        switch(type) {
        case TYPE_FLAG:
        case TYPE_INT:
            sprintf(text, "%10d", val->i_val);
            break;
        case TYPE_LINGER:
            sprintf(text, "%d:%-8d",
                val->linger_val.l_onoff, val->linger_val.l_linger);
            break;
        case TYPE_TIMEVAL:
            sprintf(text, "%6d:%-3d",
                (int)val->timeval_val.tv_sec, (int)val->timeval_val.tv_usec);
            break;
        case TYPE_STRING:
            sprintf(text, "%10s", val->c_val);
            break;
        default:
            safecopy(text, "  Ooops?  "); /* Internal error? */
        }
    }
    safeconcat(line, text);
}

static int parse_socket_option(char *arg) {
    int socket_type; /* 0-accept, 1-local, 2-remote */
    char *opt_val_str, *opt_val2_str;
    SOCK_OPT *ptr;

    if(arg[1]!=':')
        return 0; /* FAILED */
    switch(arg[0]) {
    case 'a':
        socket_type=0; break;
    case 'l':
        socket_type=1; break;
    case 'r':
        socket_type=2; break;
    default:
        return 0; /* FAILED */
    }
    arg+=2;
    opt_val_str=strchr(arg, '=');
    if(!opt_val_str) /* No '='? */
        return 0; /* FAILED */
    *opt_val_str++='\0';
    ptr=sock_opts;
    for(;;) {
        if(!ptr->opt_str)
            return 0; /* FAILED */
        if(!strcmp(arg, ptr->opt_str))
            break; /* option name found */
        ptr++;
    }
    ptr->opt_val[socket_type]=calloc(1, sizeof(OPT_UNION));
    switch(ptr->opt_type) {
    case TYPE_FLAG:
    case TYPE_INT:
        ptr->opt_val[socket_type]->i_val=atoi(opt_val_str);
        return 1; /* OK */
    case TYPE_LINGER:
        opt_val2_str=strchr(opt_val_str, ':');
        if(opt_val2_str) {
            *opt_val2_str++='\0';
            ptr->opt_val[socket_type]->linger_val.l_linger=atoi(opt_val2_str);
        } else {
            ptr->opt_val[socket_type]->linger_val.l_linger=0;
        }
        ptr->opt_val[socket_type]->linger_val.l_onoff=atoi(opt_val_str);
        return 1; /* OK */
    case TYPE_TIMEVAL:
        opt_val2_str=strchr(opt_val_str, ':');
        if(opt_val2_str) {
            *opt_val2_str++='\0';
            ptr->opt_val[socket_type]->timeval_val.tv_usec=atoi(opt_val2_str);
        } else {
            ptr->opt_val[socket_type]->timeval_val.tv_usec=0;
        }
        ptr->opt_val[socket_type]->timeval_val.tv_sec=atoi(opt_val_str);
        return 1; /* OK */
    case TYPE_STRING:
        if(strlen(opt_val_str)+1>sizeof(OPT_UNION))
            return 0; /* FAILED */
        strcpy(ptr->opt_val[socket_type]->c_val, opt_val_str);
        return 1; /* OK */
    default:
        ; /* ANSI C compiler needs it */
    }
    return 0; /* FAILED */
}

/* End of options.c */
