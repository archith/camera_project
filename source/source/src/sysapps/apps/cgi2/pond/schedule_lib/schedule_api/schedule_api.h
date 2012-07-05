
#define OUT_SCHEDULE	0
#define IN_SCHEDULE	1

/* type */
#define SCHEDULE_ALERT	0
#define SCHEDULE_ACCESS	1
/*
 * check current time to determine if now is in schedule or not
 * return	OUT_SCHEDULE - out schedule time
 *		IN_SCHEDULE - in schedule time
 *		negative value - error
 */
int schedule_chk(int type);
