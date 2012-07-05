/*write*/
#define BULK_WRITE_INIT() \
	int ret=0,errcode=0;\
	my_struct* my_ds = (my_struct*)ds;\
	my_struct* my_ds_org = (my_struct*)ds_org;\
	EQ_ERR(ds, NULL);

#define BULK_WRITE_STR_TEMP(group, item, temp, temp_org, err) \
if(ds_org == NULL || strcmp(temp, temp_org) != 0)\
{\
	fprintf(stderr, "write:%s=%s\n", item, temp);\
	ret = PRO_SetStr(group, item, temp);EQ_ERR_ERRCODE(ret, -1, err);\
}

#define BULK_WRITE_INT_TEMP(group, item, temp, temp_org, err) \
if(ds_org == NULL || temp != temp_org)\
{\
	fprintf(stderr, "write:%s=%d\n", item, temp);\
	ret = PRO_SetInt(group, item, temp);EQ_ERR_ERRCODE(ret, -1, err);\
}

#define BULK_WRITE_INT(group, item, member, err) \
	BULK_WRITE_INT_TEMP(group, item, my_ds->member, my_ds_org->member, err);

#define BULK_WRITE_STR(group, item, member, err) \
	BULK_WRITE_STR_TEMP(group, item, my_ds->member, my_ds_org->member, err)


/*read*/
#define BULK_READ_INIT() \
	my_struct* my_ds = (my_struct*)ds;\
	int ret=0,errcode=0;\
	EQ_ERR(ds, NULL);	

#define BULK_READ_STR_TEMP(group, item, temp, err) \
	ret = PRO_GetStr(group, item, temp, sizeof(temp) - 1);EQ_ERR_ERRCODE(ret, -1, err);\

#define BULK_READ_INT_TEMP(group, item, temp, err) \
	ret = PRO_GetInt(group, item, temp);EQ_ERR_ERRCODE(ret, -1, err);\

#define BULK_READ_INT(group, item, member, err) \
	BULK_READ_INT_TEMP(group, item, &my_ds->member, err);\
	//fprintf(stderr, "read:itme=%s, %s=%d\n", item, #member,my_ds->member);

#define BULK_READ_STR(group, item, member, err) \
	ret = PRO_GetStr(group, item, my_ds->member, sizeof(((my_struct*)0)->member) - 1);EQ_ERR_ERRCODE(ret, -1, err);\
	//fprintf(stderr, "read:i=%s, v=%s to member=%s(len=%d)\n", item, my_ds->member, #member, sizeof(((my_struct*)0)->member));


/*check*/
#define BULK_CHECK_INIT() \
	int errcode = 0;\
	my_struct* my_ds = (my_struct*)ds;\
	my_struct* my_ds_org = (my_struct*)ds_org;\
	EQ_ERR(ds, NULL);\
	if(my_ds_org == NULL)\
		errcode = 1;

#define BULK_CHECK_INT(member, max, min, err) \
if(my_ds_org != NULL && my_ds_org->member != my_ds->member)\
{\
	fprintf(stderr, "check: %s=%d\n", #member,my_ds->member);\
	EXP_ERR_ERRCODE_STR(my_ds->member>max || my_ds->member<min, err, \
	"value=%d max=%d min=%d", my_ds->member, max ,min);\
	errcode = 1;\
}\

#define BULK_CHECK_ENABLE(member, err) BULK_CHECK_INT(member, 1, 0, err)

#define BULK_CHECK_STR(member, err)  \
if(my_ds_org != NULL && strcmp(my_ds->member, my_ds_org->member)!=0)\
{\
	fprintf(stderr, "check: %s=%s\n", #member,my_ds->member);\
	GT_ERR_ERRCODE(strlen(my_ds->member), sizeof(((my_struct*)0)->member) - 1, err);\
	errcode = 1;\
}\

/* web msg*/
#define BULK_WEBMSG_INIT() \
	int ret;\
	char buf[1024+1];\
	message[1024]='\0';

#define BULK_WEBMSG(err, types, group, item, arg...) \
	if(err == errcode)\
	{\
		ret = WebMsg_GetStr(group, item, buf, 1024);EQ_ERR_STR(ret, -1, "webmsg:group=%s, item=%s", group, item);\
		*type=types;\
		snprintf(message, 1024, buf, ##arg);\
		return 0;\
	}


