#ifndef _EN_POOL_H_
#define _EN_POOL_H_

#include "kdef.h"
#define MAX_STR_LEN 128
#define MAX_CLIENTS 10
#define MAX_OCCUPY  10
#define POOL_NUM	50
#define POOL_EVENT_FILE	"/tmp/camera_api_event_results"
#define POOL_DATA_FILE	"/tmp/pool_data_file.txt"
enum
{
	POOL_TYPE_SERVER,
	POOL_TYPE_EVENT,
	POOL_TYPE_MESSAGE,
};
enum
{
	SERVER_DS_TYPE_HTTP,
	SERVER_DS_TYPE_SMTP,
	SERVER_DS_TYPE_IM,
	SERVER_DS_TYPE_CLIENT,
};
enum
{
	EVENT_DS_TYPE_MOTION,
	EVENT_DS_TYPE_IO_0_01,
	EVENT_DS_TYPE_IO_0_10,
	EVENT_DS_TYPE_IO_1_01,
	EVENT_DS_TYPE_IO_1_10,
};

struct server_ds
{
	int fd;
	char name[MAX_STR_LEN+1];
	char url[MAX_STR_LEN+1];
	int type;
};
struct event_ds
{
	int fd;
	char name[MAX_STR_LEN+1];
	int type;
};
struct message_ds
{
	int fd;
	char name[MAX_STR_LEN+1];
	char message[MAX_STR_LEN+1];
	char server_name[MAX_STR_LEN+1];
	char event_name[MAX_STR_LEN+1];	
	struct server_ds *server_p;
	struct event_ds *event_p;
};
struct client_ds
{
	int fd;
	int buf_index;
	char buf[MAX_STR_LEN+1];
	struct server_ds sp[MAX_OCCUPY+1];
	struct event_ds ep[MAX_OCCUPY+1];
	struct message_ds mp[MAX_OCCUPY+1];
};
void* new_ds(IN int type);
int free_ds(IN void* ds);
int free_ds_by_name(IN char* name, IN int fd, IN int type);
int construct_pool_file(void);
int init_pool(void);
int show_pool(void);
int update_pool(int type, void* ds);
int free_all_by_fd(int fd);
int save_pool_file(void);
int load_pool_file(void);
#endif

