#ifndef _WEBULK_CWC_H_
#define _WEBULK_CWC_H_
#define CWC_PASSWORD_PATTERN "********"
struct webulk_cwc_dummy
{
	int type;
	char* item;
	unsigned int member_offset;
	int member_buffer_size;
};
enum
{
	CWC_INT_TYPE,
	CWC_EBL_TYPE,
	CWC_STR_TYPE,
	CWC_PWD_TYPE,
};
int webulk_cw_enable(int enable);
int webulk_wc_enable(char* value, int* member);
int webulk_cw_string(char* str);
int webulk_wc_string(char* value, char* member, int len);
int webulk_cw_int(int value);
int webulk_wc_int(char* value, int* member);
int webulk_wc_int_int(int value, unsigned int* new_value);
int webulk_cw_ip(char* str, char* idx);
int webulk_wc_ip(char* ip, char* idx, char *value);
int webulk_cw_get(char* atkey, void* ds, const struct webulk_cwc_dummy* table);
int webulk_wc_save(char* item, char* value, char* ds, const struct webulk_cwc_dummy* table);
#define WEBULK_CWC_OFFSET(member) (unsigned int)(&((my_struct*)0)->member)
#define WEBULK_CWC_DUMMY_CAL(member) (unsigned int)(&((my_struct*)0)->member), sizeof(((my_struct*)0)->member)

#endif
