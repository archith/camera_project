
/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/

#ifndef  _PTZ_ACTION_H_
#define  _PTZ_ACTION_H_

#include "ptz_command.h"

/*------------------------------------------------------------------*/
/* purpose: Control camera On/Off    			    	    */	
/* input:   DeviceAddress, action	   	    		    */
/*------------------------------------------------------------------*/

void PTZDSwitch(byte addr,Switch action);

/*------------------------------------------------------------------*/
/* purpose: Control Iris Open/Close    			    	    */	
/* input:   DeviceAddress, action	   	    		    */
/*------------------------------------------------------------------*/

void PTZDIrisSwitch(byte addr,Iris action);

/*------------------------------------------------------------------*/
/* purpose: Control Focus Near/Far    			    	    */	
/* input:   DeviceAddress, action	   	    		    */
/*------------------------------------------------------------------*/

void PTZDFocus(byte addr,Focus action);

/*------------------------------------------------------------------*/
/* purpose: Control Zoom ZoomWide/ZoomTele   			    */	 
/* input:   DeviceAddress, action	   	    		    */
/*------------------------------------------------------------------*/

void PTZDZoom(byte addr,Zoom action);

/*------------------------------------------------------------------*/
/* purpose: Control Tilt  		  			    */	 
/* input:   DeviceAddress, action, speed  	    		    */
/*------------------------------------------------------------------*/

void PTZDTilt(byte addr,Tilt action,byte speed);

/*------------------------------------------------------------------*/
/* purpose: Control Pan  		  			    */	 
/* input:   DeviceAddress, action, speed  	    		    */
/*------------------------------------------------------------------*/

void PTZDPan(byte addr,Pan action,byte speed);

/*------------------------------------------------------------------*/
/* purpose: Control Pan and Tilt	  			    */	 
/* input:   DeviceAddress, action, speed  	    		    */
/*------------------------------------------------------------------*/

void PTZDPanTilt(byte addr,Pan panaction,byte panspeed,Tilt tiltaction,byte tiltspeed);

/*------------------------------------------------------------------*/
/* purpose: Control Stop  		  			    */	 
/* input:   DeviceAddress 		 	    		    */
/*------------------------------------------------------------------*/

void PTZDStop(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Control scan auto/manual	  			    */	 
/* input:   DeviceAddress 		 	    		    */
/*------------------------------------------------------------------*/

void PTZDScan(byte addr,Scan scan);

/*------------------------------------------------------------------*/
/* purpose: Set/Clear/Goto Preset position  			    */	 
/* input:   DeviceAddress, preset,action	 	   	    */
/*------------------------------------------------------------------*/

void PTZDPreset(byte addr,byte preset,PresetAction action);

/*------------------------------------------------------------------*/
/* purpose: ZeroPan Postion		  			    */	 
/* input:   DeviceAddress			 	   	    */
/*------------------------------------------------------------------*/

void PTZDZeroPanPosition(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Remote reset		  			    */	 
/* input:   DeviceAddress			 	   	    */
/*------------------------------------------------------------------*/

void PTZDRemoteRest(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Reset Camera		  			    */	 
/* input:   DeviceAddress			 	   	    */
/*------------------------------------------------------------------*/

void PTZDRestCamera(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Query			  			    */	 
/* input:   DeviceAddress			 	   	    */
/*------------------------------------------------------------------*/

void PTZDQuery(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Run Patrol			  			    */	 
/* input:   DeviceAddress and char *seq			 	    */
/*------------------------------------------------------------------*/

void PTZDRunPatrol(byte addr,char *seq);

/*------------------------------------------------------------------*/
/* purpose: Run AutoPan			  			    */	 
/* input:   DeviceAddress			 	   	    */
/*------------------------------------------------------------------*/

void PTZDRunAutoPan(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Set/Clear/Goto Preset position  			    */	 
/* input:   DeviceAddress, preset,action	 	   	    */
/*------------------------------------------------------------------*/

void PTZPPreset(byte addr,byte preset,PresetAction action);

/*------------------------------------------------------------------*/
/* purpose: Control Tilt  		  			    */	 
/* input:   DeviceAddress, action, speed  	    		    */
/*------------------------------------------------------------------*/

void PTZPTilt(byte addr,Tilt action,byte speed);

/*------------------------------------------------------------------*/
/* purpose: Control Pan  		  			    */	 
/* input:   DeviceAddress, action, speed  	    		    */
/*------------------------------------------------------------------*/

void PTZPPan(byte addr,Pan action,byte speed);

/*------------------------------------------------------------------*/
/* purpose: Control Stop  		  			    */	 
/* input:   DeviceAddress 		 	    		    */
/*------------------------------------------------------------------*/

void PTZPStop(byte addr);

/*------------------------------------------------------------------*/
/* purpose: Send string to RS485 	  			    */	 
/* input:   string 		 	    		    	    */
/*------------------------------------------------------------------*/

void UserByPass(char *s);

#endif
