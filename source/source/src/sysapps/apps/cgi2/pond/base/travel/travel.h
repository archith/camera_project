#ifndef _TRAVEL_H_
#define _TRAVEL_H_

#define TRAVEL_BUF_SIZE 		2048

enum
{
	TRAVEL_RET_CONTINUE,
	TRAVEL_RET_STOP,
	TRAVEL_RET_NEXT_STATE,
	TRAVEL_RET_ERROR=-1,
};
struct travel_callback_ds
{
	char* str;
	int str_len;
	int total_len;
	int terminal;
	int offset;
	int* key_idx;
	char* key;
};
typedef int (*travel_fun)(struct travel_callback_ds tcds, void* priv);
struct travel_ds
{
	char *path;
	char* key;
	int key_idx;
	travel_fun *key_fun;
	int start_offset;
	int end_offset;
	void *priv;
};
int travel_pattern(struct travel_ds ds);

#endif

