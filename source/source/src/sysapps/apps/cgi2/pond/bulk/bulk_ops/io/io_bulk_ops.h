/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*	Use of this software is restricted to the terms and
*	conditions of SerComm's software license agreement.
*
*			www.sercomm.com
****************************************************************/
#ifndef _IO_BULK_OPS_H_
#define _IO_BULK_OPS_H_

#include "ioport_global.h"

#define IO_BULK_NAME	"IO"

/* manual button type used in cgi */
#define IO_BUTTON_LEVEL	0	// change level
#define IO_BUTTON_PULSE	1	// activate pulses

/* manual button mapping used in cgi */
#define IO_BUTTON_HI	0	// "On" button maps to high
#define IO_BUTTON_LO	1	// "On" button maps to low

/* return message */
#define IO_EREADCONF	-1	// fail to read configuration file
#define IO_EWRITECONF	-2	// fail to write configuration file 
#define IO_EVALUE	-3	// invalid configuration value

/* time unit used in UI (msec) (**The time unit used in API is msec) */
#define IO_TIME_UNIT	1

/* configurations of input ports */
typedef struct io_input_conf{
	int enable;	// port switch: IO_OFF, IO_ON
        int cond;	// interrupt condition:
			//	IO_LO, IO_HI, 
			//	IO_H2L, IO_L2H
}io_input_conf;

/* configurations of output ports */
typedef struct io_output_conf{
        int normal;	// normal state :
			//	IO_LO, IO_HI
}io_output_conf;

typedef struct io_act_conf{
	int act;	// action:
			//	IO_LO, IO_HI, IO_CONF_PULSE
	int time;	// pulse time: (while action is activate pulse)
			// 	10~10000 (10 msec ~ 10 sec )
}io_act_conf;

typedef struct io_manual_conf{
	int buttype;	// manual action type:
			//	IO_BUTTON_LEVEL, IO_BUTTON_PULSE
	int map;	// manual button mapping: 
			//	(while buttype is IO_BUTTON_LEVEL)
			//	IO_BUTTON_HI: "On" button maps to high
			//	IO_BUTTON_LO: "On" button maps to low
	int time;	// manual pulse time: 
			//	(while buttype is IO_BUTTON_PULSE)
			// 	10~10000 (10 msec ~ 10 sec )
}io_manual_conf;

typedef struct io_current_state{
	int in[IO_NUM_INPUT];	// input current state:
				// IO_LO, IO_HI
	int out[IO_NUM_OUTPUT];	// output current state:
				// IO_LO, IO_HI
}io_current_state;

struct IO_DS{
	io_input_conf	in[IO_NUM_INPUT];
	io_output_conf	out[IO_NUM_OUTPUT];
	io_act_conf	act[IO_NUM_OUTPUT];
	io_manual_conf	man[IO_NUM_OUTPUT];
	io_current_state	stat;
};


int IO_BULK_ReadDS(void* ds);
int IO_BULK_CheckDS(void* ds, void* ds_org);
int IO_BULK_WriteDS(void* ds, void* ds_org);
int IO_BULK_RunDS(void* ds, void* ds_org);
int IO_BULK_WebMsg(int errcode, char* message, int* type);
#endif
