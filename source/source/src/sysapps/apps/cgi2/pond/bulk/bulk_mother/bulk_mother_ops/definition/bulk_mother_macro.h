#include <string.h>
#define BULK_MOTHER_CHECK_INT(member, arg...) \
if(my_ds_org == NULL || my_ds->member != my_ds_org->member) \
{\
	ret = bulk_mother_funcat(0, ##arg);EQ_ERR(ret, -1);\
}

#define BULK_MOTHER_CHECK_STR(member, arg...) \
if(my_ds_org == NULL || strcmp(my_ds->member, my_ds_org->member) != 0) \
{\
	ret = bulk_mother_funcat(0, ##arg);EQ_ERR(ret, -1);\
}
