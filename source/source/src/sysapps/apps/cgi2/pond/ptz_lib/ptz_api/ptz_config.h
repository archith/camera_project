/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _PTZ_CONFIG_H_
#define  _PTZ_CONFIG_H_

#define COM_OK		0
#define COM_ERROR	-1

#define PTZ_OK		0
#define PTZ_ERROR	-1

#define SC_PT_PRESET_NAME_LEN			32
#define SC_PT_PRESET_POINT_STR_LEN		32
#define SC_PT_PRESET_NUM			9
#define SC_PT_PATROL_SEQ_NUM			SC_PT_PRESET_NUM
#define SC_PT_SEQ_CONTENT_STR_LEN		((SC_PT_PRESET_POINT_STR_LEN+1)*SC_PT_PATROL_SEQ_NUM)

typedef struct COMConfig {
	int mode;		/* 0:disable,1:enable */
	int baud;		/* 0:1200,1:2400,2:4800,3:9600,4:19200
				   5:38400,6:57600,7:115200 */
	int data;		/* 0:7bits,1:8bits */
	int stop;		/* 1:1 stop bit,2:2 stop bits */
	int parity;		/* 0:Odd,1:Even,2:None,3:Mark,4:Space */
	int address;		/* Pelco address */
} COMConfig;

typedef struct PanTiltPresetCfg {
	char name[SC_PT_PRESET_NAME_LEN+1];
	char point_str[SC_PT_PRESET_POINT_STR_LEN+1];
} PanTiltPresetCfg;

typedef struct PanTiltConf {
	PanTiltPresetCfg presets[SC_PT_PRESET_NUM];
	char seq_content[SC_PT_SEQ_CONTENT_STR_LEN+1];
} PanTiltConf;

/*------------------------------------------------------------------*/
/* purpose: Read COM port configuration data from the configuration */	
/*	    file 						    */
/* input:   COMConfig						    */
/* return:  COM_OK - success					    */	
/*  	    COM_ERROR -failure					    */
/*------------------------------------------------------------------*/

int COMReadConfigData(COMConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Write COM port configuration data to configuration file */
/* input:   COMConfig						    */
/* return:  COM_OK - success					    */	
/* 	    COM_ERROR -failure					    */
/*------------------------------------------------------------------*/

int COMWriteConfigData(COMConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Read Preset configuration data from the configuration   */	
/*	    file 						    */
/* input:   PanTiltConf						    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int ReadPanTiltConf(PanTiltConf *pconf);

/*------------------------------------------------------------------*/
/* purpose: Write Preset configuration data to configuration file   */ 	
/* input:   PanTiltConf						    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int WritePanTiltConf(PanTiltConf *pconf);

/*------------------------------------------------------------------*/
/* purpose: Delete single Preset from device and configuration 	    */ 	
/* input:   char *						    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int DeleteSinglePreset(char *pn);

/*------------------------------------------------------------------*/
/* purpose: Add single Preset to device and configuration 	    */ 	
/* input:   char *						    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int AddSinglePreset(char *pn);

/*------------------------------------------------------------------*/
/* purpose: Set current position to home		 	    */ 	
/* input:   void						    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int SetUserDefinedHome(void);

/*------------------------------------------------------------------*/
/* purpose: Read Pan/Tilt speed				 	    */ 	
/* output:  *x:Pan speed, *y:Tilt speed				    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int ReadPanTiltSpeed(int *x,int *y);

/*------------------------------------------------------------------*/
/* purpose: Write Pan/Tilt speed				    */ 	
/* input:   x:Pan speed, y:Tilt speed				    */
/* return:  PTZ_OK - success					    */	
/*  	    PTZ_ERROR -failure					    */
/*------------------------------------------------------------------*/

int WritePanTiltSpeed(int x,int y);

#endif	/* _PTZ_CONFIG_H_ */
