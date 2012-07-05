/* This H file is for ioport API and ioport driver */


/* IOCTL command */
#define IOCTL_SWITCH_INPUT	1	// turn on/off input port
#define IOCTL_SWITCH_OUTPUT	2	// turn on/off output port
#define IOCTL_SET_INPUT		3	// setup input port
#define IOCTL_WAIT_INPUT	4	// listen to input port
#define IOCTL_SET_OUTPUT	5	// setup output port
#define IOCTL_ACT_OUTPUT	6	// activate output port
#define IOCTL_CHECK_STATE	7	// check I/O port state
#define IOCTL_RESET_INPUT	8	// reset input port var
#define IOCTL_RESET_OUTPUT	9	// reset output port var


struct	_io_input_conf {
	int cond;
/*-- the input trigger condition, also the secondary switch of the input
	IO_HI		1	-- rising and high level
	IO_LO		2	-- falling and low level
	IO_L2H		3	-- rising
	IO_H2L		4	-- falling
	IO_EDGE		5	-- both rising and falling
*/
};


struct	_io_output_conf {
	int normal;
/*-- the normal output value, also the secondary switch of the output
	IO_HI		1	-- high
	IO_LO		2	-- low
*/
};

struct _io_status{
	int switch_in[IO_NUM_INPUT];	// input switch
	int switch_out[IO_NUM_OUTPUT];	// output switch
	int stat_in[IO_NUM_INPUT];	// input port status
	int stat_out[IO_NUM_OUTPUT];	// output port status
};


struct _io_output_action{
	int act[IO_NUM_OUTPUT];
/*	output action:
	 	IO_HI		1	-- high
		IO_LO		2	-- low
		IO_PPULSE	6	-- positive pulse
		IO_NPULSE	7	-- negative pulse
*/	
	int time[IO_NUM_OUTPUT];
/*-- the output pulse duration, used when action is set to IO_PULSE.(unit: ms)
		10 to 10000		-- 10ms to 10s
		0			-- not pulse
*/
	int ret[IO_NUM_OUTPUT];
/*	return message for each action:
		IO_OK		0	-- ok
		IO_EVALUE	-3	-- invalid action
		IO_EBUSY	-4	-- pulse has been activating
		IO_EHI		-5	-- high but action is ppulse
		IO_ELO		-6	-- low but action is npulse
		IO_EOFF		-7	-- the port switch is off
*/
};

