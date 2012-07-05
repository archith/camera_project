#ifndef CPE_TS_H
#define CPE_TS_H

#ifdef PS2_VERSION

	typedef struct 
	{
		unsigned char flags;
		signed char x;
		signed char y;
	
	} TS_EVENT;

#else

	typedef struct 
	{
		int flags;
		int x;
		int y;
		
	} TS_EVENT;
	
#endif

#define EVENT_BUFSIZE 128


typedef struct
{
	int x;
	int y;
}Point;


typedef struct
{
	int x_u;				// (right.x - left.x)
	int x_l;				// (right_raw.x - left_raw.x)

	int y_u;
	int y_l;

	Point left;
	Point left_raw;
	Point right;
	Point right_raw;

}TS_CAL;


typedef struct
{	
 	int front;
 	int rear;
 	int size;
	TS_EVENT event_buf[EVENT_BUFSIZE];

} Queue;


typedef struct 
{
	Point raw_point;					// 剛才按 touch panel 的 (x, y) 及時間
	int last_jiffies;
	
	Point cur_point;					// x_raw 轉成的 panel 上的 (x, y) 位置
	TS_CAL cal;                       	// Calibration values

	Queue event_queue;
	struct fasync_struct *fasync;     	// asynch notification
	struct timer_list debounce_timer;      	// Timer for triggering acquisitions
	struct timer_list release_timer;    // Timer for triggering release mouse
	wait_queue_head_t wait;           	// read wait queue
	spinlock_t lock;
	int mouse_flags;					// 1 ==> mouse pressed, 0 ==> mouse release
	int irq;
	unsigned long io_base;
} cpe_ts_t;


// --------------------------------------------------------------------
//	cpe_ts_t->mouse_flags
// --------------------------------------------------------------------
#define MOUSE_PRESS			1
#define MOUSE_RELEASE		0




#endif
