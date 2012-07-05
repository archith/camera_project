#ifndef _IOAPI_H_
#define _IOAPI_H_

#include "ioport_global.h"             

/* device type */
#define IO_IN		1	// input device
#define IO_OUT		2	// output device

/* I/O condition/action */
#define IO_HI		1	// high
#define IO_LO		2	// low
#define IO_L2H		3	// rising
#define IO_H2L		4	// falling
#define IO_EDGE		5	// any edge
#define IO_PPULSE	6	// positive pulse
#define IO_NPULSE	7	// negative pulse

/* return message: api return */
#define IO_OK		0	// success
#define IO_EOPENDEV	-1	// fail to open device
#define IO_ECTLDEV	-2	// fail to control device
#define IO_EVALUE	-3	// invalid value


/* device file location */
#define IO_DEVFILE	"/dev/ioport0"
#define IO_DEVFILE1	"/dev/ioport1"
#define IO_DEVFILE2	"/dev/ioport2"
#define IO_DEVFILE3	"/dev/ioport3"
#define IO_DEVFILE4	"/dev/ioport4"

#ifdef IOPORT_ADDOUT
#define IO_DEVFILE5	"/dev/ioport5"
#define IO_DEVFILE6	"/dev/ioport6"
#endif


/* default setup value */
#define IO_IN_COND_DEF		IO_H2L	// input trigger condition is falling edge
#define IO_OUT_NORM_DEF		IO_LO	// output normal state is low
#define IO_OUT_DUR_DEF		50	// output pulse duration is 500ms

/* input port settings */
typedef struct io_input_t{
	int num;	// port number: IO_NUM1, IO_NUM2
	int cond;	// obsolete element, ignore it please
			// interrupt condition:
			//	IO_EDGE
}io_input_t;


/* output port actions */
typedef struct io_output_t{
	int num;	// port number: IO_NUM1, IO_NUM2
	int act;	// action:
			//	IO_HI, IO_LO, IO_PPULSE, IO_NPULSE
}io_output_t;


/* output port action */
typedef struct io_action_t{
	io_output_t out;	// output port actions
	int time;		// pulse time:
				//	10~10000 (unit: ms)
	int ret;		// action return value:
				// 	IO_OK, IO_EVALUE, IO_EBUSY, 
				// 	IO_EHI, IO_ELO, IO_EOFF
}io_action_t;

typedef struct io_stat_t{
	int type;	// device type of the port
			// 	IO_IN, IO_OUT
	int num;	// port number
			//	IO_NUM1, IO_NUM2
	int enable;	// port switch
			//	IO_OFF, IO_ON
	int stat;	// port status
			//	IO_HI, IO_LO
}io_stat_t;


/* Function:	IO_StopInput
 * Description:	1. turn off the switch of each input port
 * Parameter:	(IN) num:
 *			specify the device number of the type
 *				IO_NUM1	1	-- port#1
 *				IO_NUM2	2	-- port#2
 *
 * Return:	IO_OK		0	// success
 *		IO_EOPENDEV	-1	// fail to open device
 *		IO_ECTLDEV	-2	// fail to control device
 *		IO_EVALUE	-3	// invalid input value
 */
int IO_StopInput(int num);


/* Function:	IO_StopOutput
 * Description:	1. turn off the switch of each output port
 * Parameter:	(IN) num:
 *			specify the device number of the type
 *				IO_NUM1	1	-- port#1
 *				IO_NUM2	2	-- port#2
 *
 * Return:	IO_OK		0	// success
 *		IO_EOPENDEV	-1	// fail to open device
 *		IO_ECTLDEV	-2	// fail to control device
 *		IO_EVALUE	-3	// invalid input value
 */
int IO_StopOutput(int num);


/* Function:	IO_SetInputFasync
 * Description:	Open input port[no] and set fasync.
 * 		Caller will receive signal 'sig' when
 * 		input state changes.
 * Parameter:	(IN) no - port number
 *			IO_NUM1
 *			IO_NUM2
 *		(IN) sig - signal number
 * Return:	-1 - error
 *		others - file descriptor
 * Note:	Remember to close the 
 *		file descriptor
 */
int IO_SetInputFasync(int no, int sig);


/* Function:	IO_SetOutput
 * Description:	setup output configuration to the device
 * Parameter:	(IN) output_s:
 *			pointer of a structure containing
 *			the output configuration details.
 * Return:	IO_OK           0       // success
 *		IO_EOPENDEV     -1      // fail to open device
 *		IO_ECTLDEV      -2      // fail to control device
 *		IO_EVALUE       -3      // invalid input value
 */
int IO_SetOutput(io_output_t* output_s);


/* Function:	IO_TrigOutput
 * Description:	1. trigger output;
 *		2. the actions of multiple ports can be activate
 *		   at one time;
 *		3. the actions are not related to the normal states
 *		4. the errors of each port will be saved in the 
 *		   variable "act_s[].ret"
 * 		5. if a port number repeats, the last action will 
 *		   be executed
 *		6. size of memory allocated by user: nmem*sizeof(io_action_t).
 *
 * Parameter:	(IN) act_s[].out.num
 *		(IN) act_s[].out.act
 *		(IN) act_s[].time
 *		(IN) nmem:
 *			number of the array members
 *		(OUT) act_s[].ret
 *
 * Return:	IO_OK		0	// success
 *		IO_EOPENDEV	-1	// fail to open device
 *		IO_ECTLDEV	-2	// fail to control device
 *		IO_EVALUE	-3	// invalid input value
 */
int IO_TrigOutput(io_action_t act_s[], int nmem);


/* Function:	IO_CheckState
 * Description: 1. check current state of specified input and output.
 *		2. calling function must allocate memory first.
 *		3. size of memory allocated by user: nmem*sizeof(io_stat_t).
 * Parameter:	(IN) stat_s[].type
 *		(IN) stat_s[].num
 *		(IN) nmem:
 *			number of array members
 *			1 ~ (amount of all I/O ports)
 *		(OUT) stat_s[].enable
 *		(OUT) stat_s[].stat
 *
 * Return:	IO_OK		0	// ok.
 *		IO_EOPENDEV	-1	// fail to open device
 *		IO_ECTLDEV	-2	// fail to control device
 *		IO_EVALUE	-3	// invalid input value
 */
int IO_CheckState(io_stat_t stat_s[], int nmem);



/* Function:	IO_InitInput
 * Description:	//obsolete:(1. setup input configuration to the device)
 *		2. enable input port
 * Parameter:	(IN) input_s:
 *		 	pointer of a structure containing
 *			the input settings.
 * Return:	IO_OK		0	// success
 *		IO_EOPENDEV	-1	// fail to open device
 *		IO_ECTLDEV	-2	// fail to control device
 *		IO_EVALUE	-3	// invalid input value
 */
int IO_InitInput(io_input_t* input_s);


/* Function:	IO_InitOutput
 * Description:	1. reset output variable in I/O driver
 *		2. setup output configuration to the device
 *		3. enable output port
 * Parameter:	(IN) output_s:
 *			pointer of a structure containing
 *			the output settings.
 * Return:	IO_OK		0	// success
 *		IO_EOPENDEV	-1	// fail to open device
 *		IO_ECTLDEV	-2	// fail to control device
 *		IO_EVALUE	-3	// invalid input value
 */
int IO_InitOutput(io_output_t* output_s);
#endif
