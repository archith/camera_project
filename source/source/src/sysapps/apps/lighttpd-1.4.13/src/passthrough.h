
typedef struct passthrough_status{
	int auth_id;
	time_t last_time;
	buffer authinfo;// the authorization info of this cookie
}PASSTHROUGH_STATUS;

#define PASSTHROUGH_NUM		10
typedef struct passthrough_lock{
	PASSTHROUGH_STATUS status[PASSTHROUGH_NUM];
	int lock_idx;
	char cookie_name[20];
}PASSTHROUGH_LOCK;

int get_auth_from_string(buffer* buf, char* str);
int insert_query_auth_in_request_headers(server* srv, connection* con);

int get_auth_cookie_name(server* srv);
int set_auth_cookie_in_response(server* srv, connection* con);
int check_auth_cookie(server* srv, connection* con);
int get_cookie_authinfo(server *srv, connection *con);

int del_passthrough_authinfo(server *srv, connection *con);
int passthrough_lock(server* srv, connection* con, char* authstr);
int response_passthrough_max(server* srv, connection* con);
