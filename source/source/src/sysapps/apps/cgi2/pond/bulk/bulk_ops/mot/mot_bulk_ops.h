/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _MOT_BULK_OPS_H_
#define  _MOT_BULK_OPS_H_
#define MOT_DS_NAME_LEN 	14
#define MOT_DS_WINDOW_LEN	32

#define MOT_BULK_NAME	SEC_MOTION

#define  RES160         0  /* 120 x 160 */
#define  RES320         1  /* 240 x 320, default */
#define  RES640         2  /* 480 x 640 */
/* Resolution */
#define IMG_RES_120     1  /* 160 x 120 */
#define IMG_RES_240     2  /* 320 x 240 */
#define IMG_RES_480     3  /* 640 x 480 */
#define IMG_RES_NUM     4
#define  AREA_NUM       4
#define MOTION_DEF_SENS         5
#define SCHEDULE_LEN            256

struct area_str {
        int     en;             // The motion area enbale[1]/disable[0].
        int     thd;            // threshold (0~255).
        int     thdIdx;         // threshold level Index(0~10).
        int     thdVal;         // threshold level value(from table).
        int     sens;           // sensitivity(0~10).
        int     sensIdx;        // sensitivity level Index(0~10).
        int     sensVal;        // sensitivity level Value(from table).
        char    name[14];       // window title(area name)
        char    coord[32];
        int     resXY[IMG_RES_NUM][4];  // The area coordinate(X1/Y1,X2/Y2).
        int     resXYBlk[IMG_RES_NUM][4];       // (X1/Y1,X2/Y2) in block unit.
        int     indcator;          // indicator.
        int     blkNum;         // Block num of the area.
};

struct motion_str {
        int     en;             //Motion enable[1]/disable[0]
        int     res;            //Image resolution[0..2]
        int     interval;       //Motion interval
        int     resBlkRow[IMG_RES_NUM];//Block number in a Row.
        int     resBlkNum[IMG_RES_NUM];//Total block number in three resulotion
        struct  area_str area[AREA_NUM];//Detecting areas
        char    alert_schedule[SCHEDULE_LEN];
                                // format day start end: day start end
                                // day: 1~ 7 : sun ~ sat
                                //      8 : weekdays ( mon ~ fri )
                                //      9 : everyday in week
                                // eg 809001800:609001200
                                // weekday from 9 to 18 & Sat from 9
        int pt_md_mutex;      // disable mot while position incorrect
        char ptcoord[6];        //PT position when save motion conf
};

int ReadMotConf(void *conf);
int Mot_CheckConf(void* ds, void* ds_org);
int Mot_WriteConf(void *conf, void *conf_org);
int SaveMotConf(void *conf);
int Mot_RunConf(void *conf, void *conf_org);
#endif
