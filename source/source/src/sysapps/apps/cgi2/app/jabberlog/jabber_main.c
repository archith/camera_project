/* iksemel (XML parser for Jabber)
** Copyright (C) 2000-2004 Gurer Ozen <madcat@e-kolay.net>
** This code is free software; you can redistribute it and/or
** modify it under the terms of GNU Lesser General Public License.
*/

#include "common.h"
#include "iksemel.h"
#include <im_bulk_ops.h>
#include <conf_sec.h>
#include <profile.h>
#define max(a,b) (a>b?a:b)
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <im_api.h>
#include <log_api.h>
#include <base_tools.h>
#include <ipc_tools.h>
#include <sys/file.h>
#include <model_conf.h>
/* stuff we keep per session */
struct session {
	iksparser *prs;
	iksid *acc;
	char *pass;
	int features;
	int authorized;
	int counter;
	int set_roster;
	int job_done;
};

/* precious roster we'll deal with */
iks *my_roster;

/* out packet filter */
iksfilter *my_filter;

/* connection time outs if nothing comes for this much seconds */
int opt_timeout = 30;

/* connection flags */
int opt_use_tls=0;
int opt_use_sasl=1;
int opt_log=0;

enum
{
	JL_OK,
	JL_NORMAL_ERROR,
	JL_AUTHEN_FAILED,
	JL_SERVER_DISCONNECTED,
	JL_CANNOT_CONNECT,

};
void
j_error (char *msg)
{
	fprintf (stderr, "jabber main: %s\n", msg);
	//exit (2);
}

int
on_result (struct session *sess, ikspak *pak)
{
	iks *x;

	if (sess->set_roster == 0) {
		x = iks_make_iq (IKS_TYPE_GET, IKS_NS_ROSTER);
		iks_insert_attrib (x, "id", "roster");
		iks_send (sess->prs, x);
		iks_delete (x);
	} else {
		iks_insert_attrib (my_roster, "type", "set");
		iks_send (sess->prs, my_roster);
	}
	return IKS_FILTER_EAT;
}

int
on_stream (struct session *sess, int type, iks *node)
{
	sess->counter = opt_timeout;

	switch (type) {
		case IKS_NODE_START:
			if (opt_use_tls && !iks_is_secure (sess->prs)) {
				iks_start_tls (sess->prs);
				break;
			}
			if (!opt_use_sasl) {
				iks *x;

				x = iks_make_auth (sess->acc, sess->pass, iks_find_attrib (node, "id"));
				iks_insert_attrib (x, "id", "auth");
				iks_send (sess->prs, x);
				iks_delete (x);
			}
			break;

		case IKS_NODE_NORMAL:
			if (strcmp ("stream:features", iks_name (node)) == 0) {
				sess->features = iks_stream_features (node);
				if (opt_use_sasl) {
					if (opt_use_tls && !iks_is_secure (sess->prs)) break;
					if (sess->authorized) {
						iks *t;
						if (sess->features & IKS_STREAM_BIND) {
							t = iks_make_resource_bind (sess->acc);
							iks_send (sess->prs, t);
							iks_delete (t);
						}
						if (sess->features & IKS_STREAM_SESSION) {
							t = iks_make_session ();
							iks_insert_attrib (t, "id", "auth");
							iks_send (sess->prs, t);
							iks_delete (t);
						}
					} else {
						if (sess->features & IKS_STREAM_SASL_MD5)
							iks_start_sasl (sess->prs, IKS_SASL_DIGEST_MD5, sess->acc->user, sess->pass);
						else if (sess->features & IKS_STREAM_SASL_PLAIN)
							iks_start_sasl (sess->prs, IKS_SASL_PLAIN, sess->acc->user, sess->pass);
					}
				}
			} else if (strcmp ("failure", iks_name (node)) == 0) {
				j_error ("sasl authentication failed");
				return JL_AUTHEN_FAILED;
			} else if (strcmp ("success", iks_name (node)) == 0) {
				sess->authorized = 1;
				iks_send_header (sess->prs, sess->acc->server);
			} else {
				ikspak *pak;

				pak = iks_packet (node);
				iks_filter_packet (my_filter, pak);
				if (sess->job_done == 1) return IKS_HOOK;
			}
			break;

		case IKS_NODE_STOP:
			j_error ("server disconnected");
			return JL_SERVER_DISCONNECTED;

		case IKS_NODE_ERROR:
			j_error ("stream error");
			return JL_NORMAL_ERROR;
	}

	if (node) iks_delete (node);
	return IKS_OK;
}

int
on_error (void *user_data, ikspak *pak)
{
	j_error ("authorization failed");
	return IKS_FILTER_EAT;
}

int
on_roster (struct session *sess, ikspak *pak)
{
	my_roster = pak->x;
	sess->job_done = 1;
	return IKS_FILTER_EAT;
}

void
on_log (struct session *sess, const char *data, size_t size, int is_incoming)
{
	if (iks_is_secure (sess->prs)) fprintf (stderr, "Sec");
	if (is_incoming) fprintf (stderr, "RECV"); else fprintf (stderr, "SEND");
	fprintf (stderr, "[%s]\n", data);
}

void
j_setup_filter (struct session *sess)
{
	if (my_filter) iks_filter_delete (my_filter);
	my_filter = iks_filter_new ();
	iks_filter_add_rule (my_filter, (iksFilterHook *) on_result, sess,
		IKS_RULE_TYPE, IKS_PAK_IQ,
		IKS_RULE_SUBTYPE, IKS_TYPE_RESULT,
		IKS_RULE_ID, "auth",
		IKS_RULE_DONE);
	iks_filter_add_rule (my_filter, on_error, sess,
		IKS_RULE_TYPE, IKS_PAK_IQ,
		IKS_RULE_SUBTYPE, IKS_TYPE_ERROR,
		IKS_RULE_ID, "auth",
		IKS_RULE_DONE);
	iks_filter_add_rule (my_filter, (iksFilterHook *) on_roster, sess,
		IKS_RULE_TYPE, IKS_PAK_IQ,
		IKS_RULE_SUBTYPE, IKS_TYPE_RESULT,
		IKS_RULE_ID, "roster",
		IKS_RULE_DONE);
}

int
j_connect (struct session* sess,char *jabber_id, char *pass, char* nickname)
{
	int e;

	sess->prs = iks_stream_new (IKS_NS_CLIENT, sess, (iksStreamHook *) on_stream);
	if (opt_log) iks_set_log_hook (sess->prs, (iksLogHook *) on_log);
	sess->acc = iks_id_new (iks_parser_stack (sess->prs), jabber_id);
	if (NULL == sess->acc->resource) {
		/* user gave no resource name, use the default */
		char *tmp;
		tmp = iks_malloc (strlen (sess->acc->user) + strlen (sess->acc->server) + 9 + 3);
		sprintf (tmp, "%s@%s/%s", sess->acc->user, sess->acc->server, COMPANY_NAME);
		sess->acc = iks_id_new (iks_parser_stack (sess->prs), tmp);
		iks_free (tmp);
	}
	sess->pass = pass;
	sess->set_roster = 0;
	j_setup_filter (sess);

	e = iks_connect_tcp (sess->prs, sess->acc->server, IKS_JABBER_PORT);
	switch (e) {
		case IKS_OK:
			break;
		case IKS_NET_NODNS:
			j_error ("hostname lookup failed");
			return JL_CANNOT_CONNECT;
		case IKS_NET_NOCONN:
			j_error ("connection failed");
			return JL_CANNOT_CONNECT;
		default:
			j_error ("io error");
			return JL_NORMAL_ERROR;
	}

	sess->counter = opt_timeout;
	while (1) {
		e = iks_recv (sess->prs, 1);
		if (IKS_HOOK == e) break;
		if (IKS_NET_TLSFAIL == e) 
		{
			j_error ("tls handshake failed");
			return JL_NORMAL_ERROR;
		}
		if (IKS_OK != e) 
		{
			j_error ("io error");
			return JL_NORMAL_ERROR;
		}
		sess->counter--;
		if (sess->counter == 0) 
		{
			j_error ("network timeout");
			return JL_NORMAL_ERROR;
		}
	}
	{
		iks* cc;
		cc=iks_make_pres(IKS_SHOW_AVAILABLE, nickname);
		iks_send(sess->prs, cc);
		iks_delete(cc);
	}

	return JL_OK;
}
int j_SendMessage(struct session* sess, char* jid, char* message)
{
	iks* cc;
	int ret;
	cc=iks_make_msg(IKS_TYPE_CHAT, jid, message);
	ret = iks_send(sess->prs, cc);
	iks_delete(cc);
	return ret;
}
int j_main (char* server, char* account, char* password, char* sendto, char* nickname)
{
	int ret=0,log_fd=0,j_fd=0,errcode=JL_OK;
	struct session sess;
	int maxfd;
	fd_set rset;
//	struct LogJabberConf conf;
	char buf[1024];
	struct timeval tv;
		
	/*init some values*/
//	opt_log = 1;
	memset (&sess, 0, sizeof (struct session));	
	
//	/*get values from configuration file*/
//	if(LOGReadJabberConf(&conf)!=L_OK)
//		ERROUT_ERRCODE(JL_NORMAL_ERROR);
	
	/*setup jabber client*/
	sprintf(buf, "%s@%s", account, server);
	ret = j_connect(&sess,buf, password, nickname);NE_ERR_ERRCODE(ret, JL_OK, JL_CANNOT_CONNECT);
	j_fd = iks_fd(sess.prs);
	
	/*setup connection with SYS_log*/
	log_fd = unix_DS_server(SYSLOG_IM_DAEMON);
	EQ_ERR_ERRCODE(log_fd, -1, JL_NORMAL_ERROR);

	AddLog("IM", 2, 4);//4=JABBER: Login successfully
	FD_ZERO(&rset);
	tv.tv_sec = 5*60;
	tv.tv_usec = 0;		
	for(;;)
	{
		FD_SET(log_fd, &rset);
		FD_SET(j_fd, &rset);
		maxfd = max(log_fd, j_fd)+1;
		ret = select(maxfd, &rset, NULL, NULL, &tv);
		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;
			EQ_ERR_ERRCODE(0, 0, JL_SERVER_DISCONNECTED);
		}
		/*get messages from SYS_log and send them to Jabber server*/
		if(FD_ISSET(log_fd, &rset))
		{
			ret = read(log_fd, buf, 1024);LS_ERR_ERRCODE(ret, 0, JL_SERVER_DISCONNECTED);
			buf[ret]='\0';
			ret =j_SendMessage(&sess, sendto, buf);NE_ERR_ERRCODE(ret, JL_OK, ret);
			AddLog("IM", 2, 5);//5=IM: Send message OK.
		}
		/*skip any messages from jabber server*/
		if(FD_ISSET(j_fd, &rset))
		{
			ret = read(j_fd, buf, 1024);LS_ERR_ERRCODE(ret, 0, JL_SERVER_DISCONNECTED);
			if(ret == 0)
			{
				EQ_ERR_ERRCODE(0, 0, JL_SERVER_DISCONNECTED);
			}
			if (ret < 0)
			{
				if (errno == EINTR || errno == EAGAIN)
					continue;
				EQ_ERR_ERRCODE(0, 0, JL_SERVER_DISCONNECTED);
			}
		}
		/* keep alive message*/
		if(tv.tv_sec == 0)
		{
			tv.tv_sec = 5*60;
			tv.tv_usec = 0;		
			ret = write(j_fd, "\x20", 1);LS_ERR_ERRCODE(ret, 0, JL_SERVER_DISCONNECTED);
		}
	}
errout:
	if(log_fd)
		close(log_fd);
	if(j_fd)
		close(j_fd);
	if(sess.prs)
		iks_parser_delete (sess.prs);	
	if( errcode == JL_CANNOT_CONNECT)
		AddLog("IM", 2, 2);//2=JABBER: Can't connect to server
	if(errcode == JL_AUTHEN_FAILED)
		AddLog("IM", 2, 1);//1=JABBER: Authentication failed
	if(errcode == JL_SERVER_DISCONNECTED)
		AddLog("IM", 2, 3);//3=JABBER: Server disconnected
	return errcode;
}
int create_stunnel(char* server)
{
	int fd;
	char buf[128];	
	char fix[]="foreground = no\ndebug=0\npid =\noptions = ALL\nTIMEOUTclose = 0\nclient = yes\nverify = 0\n[jabber]\naccept = 5222\n";
	system("/usr/bin/killall jabber_stunnel");	
	//make jabber_stunnel.conf
	fd = open("/tmp/jabber_stunnel.conf", O_RDWR | O_CREAT | O_TRUNC);
	if(fd < 0)
		return -1;
	write(fd, fix, sizeof(fix)-1);
	sprintf(buf, "connect = %s:5223\n", server);
	write(fd, buf, strlen(buf));
	close(fd);
	system("/usr/local/bin/jabber_stunnel /tmp/jabber_stunnel.conf >/dev/zero 2>/dev/zero");
	return 0;	
}
int main (int argc, char *argv[])
{
	int ret, fd, i = 0;
	char host_name[128+1]="HHH";
	struct IM_DS conf;

	fd = open("/tmp/jabberlog_output_file", O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IXUSR);EQ_ERR(fd, -1);	
	for(i=0;i<20;i++)
	{
		ret = flock(fd, LOCK_EX|LOCK_NB);
		if(ret != -1)
			break;
		sleep(1);
	}
	EQ_ERR(i, 20);
	ret = fcntl(fd, F_SETFD, FD_CLOEXEC);EQ_ERR(ret, -1);

	ret = IM_BULK_ReadDS((void*)&conf);EQ_ERR(ret, -1);
	EQ_ERR(conf.enable, 0);
	ret = PRO_GetStr(SEC_SYS, SYS_HOSTNAME, host_name, 128);EQ_ERR(ret, -1);
	while(1)
	{
#if SC_SSL_PROXY == 1
		ret = create_stunnel(conf.server);
#endif
		j_main(conf.server, conf.account, conf.password, conf.sendto, host_name);
		sleep(10*60);		
	}
	close(fd);
	return 0;
errout:
	if(fd != -1)
		close(fd);
	return -1;
}

