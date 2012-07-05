1.SC_SSL_PROXY
	if SC_SSL_PROXY is defined, jabberlog will connect to "127.0.0.1:5223".
	There is a daemone will listen to the port "127.0.0.1:5223".
	Each messages from jabberlog will be SSL encoed and send to the real destination.

2. opt_log
	there is a variable: opt_log.
	If the opt_log is 1, any messages from and to in jabberlog will be displayed in stdout.