#ifndef _TRAVEL_CONF_H_
#define _TRAVEL_CONF_H_
int travel_conf_format(char* group, char* item, char* value, int value_len, char* path);

typedef int (*travel_conf_group_callback)(char* group, char* item, char* value);
int travel_conf_group(char* group, char* item, int item_len, char* value, int value_len, char* path, travel_conf_group_callback callback);
#endif

