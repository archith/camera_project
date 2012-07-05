/* This H file is for ALL ioport concerned API, driver, and programs */
#ifndef _IOPORT_GLOBAL_H_
#define _IOPORT_GLOBAL_H_

/* total number of input/output */
#define IO_NUM_INPUT	2	// total number of input

#ifdef IOPORT_ADDOUT
#define IO_NUM_OUTPUT	4	// total number of output
#else
#define IO_NUM_OUTPUT	2
#endif

/* I/O device number */
#define IO_NUM1		1	// port#1
#define IO_NUM2		2	// port#2

/* I/O condition/action */
#define IO_UNKNOWN	-100	// not define
#define IO_HI		1	// high
#define IO_LO		2	// low
#define IO_L2H		3	// rising (in)
#define IO_H2L		4	// falling (in)
#define IO_EDGE		5	// rising or falling (in)
#define IO_PPULSE	6	// positive pulse (out)
#define IO_NPULSE	7	// negative pulse (out)
#define IO_CONF_PULSE	3	// pulse (direction undefined)

/* I/O switch status */
#define IO_ON		1	// on
#define IO_OFF		0	// off


/* return message: ("ret" in struct io_action_t)*/
#define	IO_OK		0	// ok
#define	IO_EVALUE	-3	// invalid action
#define	IO_EBUSY	-4	// pulse has been activating
#define	IO_EHI		-5	// high but action is ppulse
#define	IO_ELO		-6	// low but action is npulse
#define	IO_EOFF		-7	// IOport off

/* output pulse time limit */
#define IO_PULSE_MIN	10	// pulse time (unit: ms)
#define IO_PULSE_MAX	10000


#endif	// _IOPORT_GLOBAL_H_
