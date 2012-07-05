#ifndef _WEBULK_OPS_H_
#define _WEBULK_OPS_H_

enum
{
	WEBULK_AUTH_FLAG_USER,
	WEBULK_AUTH_FLAG_ADMIN,
};
struct webulk_entry
{
	const char** item_list;
	const int (*get)(char* item, void* ds);
	const int (*save)(char* item, char* value, void* ds);
	int auth_flag;
};
struct webulk_ops
{
	char* bulk_ops_name;
	const struct webulk_entry* entry_list;
};

#endif

