#ifndef _PROFILE_H_
#define _PROFILE_H_

#include <kdef.h>
#define PRO_GROUP_MAX_LEN 	32
#define PRO_ITEM_MAX_LEN 	32
#define PRO_VALUE_MAX_LEN 	512
/*
Description:
	get a string from configuration file.
	The result will be put into "value" with a terminal sign '\0';
	"value_len" indicate the buffer space of "value". NOT include terimal sign '\0'
	EX)
		if "value_len" is 20, PRO_GetStr will put no more than 21 characters into "value".
		(20 characters + 1 terminal sign '\0')
Return Value:
	-1: error
*/
int PRO_GetStr(IN char* group, IN char* item, OUT char* value, IN int value_len);
/*
Description:
	get integer value.
	The result will be put into "value";
Return Value:
	-1: error
	0: success
*/
int PRO_GetInt(IN char* group, IN char* item, IN int* value);
/*
Description:
	set a string "value" into configuration file.
Return Value:
	-1: error
*/

int PRO_SetStr(IN char* group, IN char* item, IN char* value);
/*
Description:
	set a integer "value" into configuration file.
Return Value:
	-1: error
*/

int PRO_SetInt(char* group, char* item, int value);

/*
en/decode "src" string with length "src_len".
output data will be put into "dst" with length "dst_len", not include '\0'
*/
int PRO_value_encoder(IN char* src, IN int src_len, OUT char* dst, INOUT int* dst_len);
int PRO_value_decoder(IN char* src, IN int src_len, OUT char* dst, INOUT int* dst_len);
int DEF_GetStr(IN char* group, IN char* item, OUT char* value, IN int value_len);
int DEF_GetInt(IN char* group, IN char* item, IN int* value);

typedef int (*DEF_callback)(char* group, char* item, char* value);
int DEF_GetGroup(IN char* group, DEF_callback callback);

#endif

