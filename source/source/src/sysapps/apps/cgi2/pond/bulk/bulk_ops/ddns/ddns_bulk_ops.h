/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _DDNS_BULK_OPS_H_
#define  _DDNS_BULK_OPS_H_

#define DDNS_BULK_NAME   SEC_DDNS

#define DDNS_MAX_SERVERHOST_LEN 128
#define GENERIC_STRING_LEN      32
#define DDNS_ACCOUNT_LEN        64      //48, accont in tzo is email address

#define DDNS_OK         0
#define DDNS_ERROR      -1
#define DDNS_EINVALID   -2      /* Malformat Input */

#define ENABLE          1
#define DISABLE         0

#define DDNS_UNIT_MIN   1
#define DDNS_UNIT_HR    2
#define DDNS_UNIT_DAY   3

#define DYNDNS_NAME     "0"
#define TZO_NAME        "1"
#define DHS_NAME        ""
#define LINKSYS_NAME    "5"
#define CHNAGEIP_NAME   ""

/****************************************************************
 * The usage of schedule can be viewed as the the unit of time,
 * and the interval is the value of that unit. For example,
 * schedule=DDNS_UNIT_MIN,interval=20 -> update every 20 minutes
 * schedule=DDNS_UNIT_HOUR,interval=6 -> update every 6 hours
 *****************************************************************
 * The start_hour and the start_min is used to indicate the
 * starting time, the range of hour is from 0 to 23 , and the range
 * of minute is from 0 to 59
 *****************************************************************/
typedef struct ddns_param
{
  int enable;                           /* flag to indicate en/dis-able  */
  char server[GENERIC_STRING_LEN+1];    /* service providing server name */
                                        /* server must be                *
                                         * DYNDNS_NAME or TZO_NAME       */
  char account[DDNS_ACCOUNT_LEN+1];     /* username / account            */
  char password[GENERIC_STRING_LEN+1];  /* passowrd for authentication   */
  char hostname[DDNS_MAX_SERVERHOST_LEN+1]; /* domain name               */
  int schedule;                         /* scheduling unit               */
  int interval;                         /* scheduling interval           */
  int start_hour;                       /* starting hour 0 - 23          */
  int start_min;                        /* starting minutes 0 - 59       */
}ddns_param_t;

int ddns_ReadConf(void *conf);
int ddns_BULK_CheckDS(void* ds, void* ds_org);
int ddns_WriteConf(void *conf, void *conf_org);
int ddns_RunConf(void *conf, void *conf_org);
#endif
