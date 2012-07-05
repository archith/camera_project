// --------------------------------------------------------------------
//	2003-02-26: lmc83
//		- modified from au1000_ts.c
// --------------------------------------------------------------------
#define UINT32 unsigned int
#define VA_CPE_IC_BASE  CPE_IC_VA_BASE
#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/ioport.h>       /* request_region */
#include <linux/interrupt.h>    /* mark_bh */
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>        /* get_user,copy_to_user */
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/arch/ssp.h>
#include <asm/arch/irq.h>
#include "cpe_ts.h"

//modified by ivan
//#ifdef CONFIG_CPE_ASIC_BOARD
//
#define CONFIG_CPE120_PLATFORM 1

#ifdef CONFIG_CPE110_PLATFORM
#define TOUCH_PANEL_RIGHT_X  320
#define TOUCH_PANEL_RIGHT_Y  240
#define IRQ_TOUCH       21
//#elif CONFIG_CPE_FPGA_BOARD
#elif CONFIG_CPE10x_PLATFORM
#define TOUCH_PANEL_RIGHT_X 640
#define TOUCH_PANEL_RIGHT_Y 480 
#define IRQ_TOUCH       21
#elif CONFIG_CPE120_PLATFORM
#define TOUCH_PANEL_RIGHT_X  320
#define TOUCH_PANEL_RIGHT_Y  240
#define IRQ_TOUCH       28
#else
#error "no board support"
#endif

#define VA_CPE_SSP1_BASE     IO_ADDRESS(CPE_SSP1_BASE)
#define VA_CPE_PWM_BASE      IO_ADDRESS(CPE_PWM_BASE)
/* end add */
///#define PS2_VERSION

#define TS_NAME 	"cpe-ts"

#ifdef PS2_VERSION
	#define TS_MINOR    1
#else
	#define TS_MINOR    9
#endif


#define PwDn_InEn_DFR12		0x90
#define X_DFR12				0x90 /*FPGA org 0x92*/
#define Y_DFR12				0xD0 /*FPGA org 0xD2*/

#define DEBOUNCE_TIME		5						// 下一個 interrupt event 若是在 DEBONCE_TIME 之內發生, 則算是同一個按下的 event
#define RELEASE_TIME		13



#define PFX TS_NAME
#define AU1000_TS_DEBUG 1

#ifdef AU1000_TS_DEBUG
#define dbg(format, arg...) printk(KERN_DEBUG PFX ": " format "\n" , ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) printk(KERN_ERR PFX ": " format "\n" , ## arg)
#define info(format, arg...) printk(KERN_INFO PFX ": " format "\n" , ## arg)
#define warn(format, arg...) printk(KERN_WARNING PFX ": " format "\n" , ## arg)

static cpe_ts_t cpe_ts;



// --------------------------------------------------------------------
//		(new.x - left.x)/(new_raw.x - left_raw.x) = (right.x - left.x)/(right_raw.x - left_raw.x)
//		==> new.x = (new_raw.x - left_raw.x) * (right.x - left.x) / (right_raw.x - left_raw.x) + left.x
//		==> new.x = ratio_x * (new_raw.x - left_raw.x) + left.x
// --------------------------------------------------------------------

// --------------------------------------------------------------------
//			support instructions
// --------------------------------------------------------------------


#define xtod(c)         ((c) <= '9' ? '0' - (c) : 'a' - (c) - 10)
long
atoi(register char *p)
{
	register long n;
    register int c, neg = 0;

    if (p == NULL)
		return 0;

    if (!isdigit(c = *p)) 
    {
        while (isspace(c))
                c = *++p;
        switch (c) {
        case '-':
                neg++;
        case '+': /* fall-through */
                c = *++p;
        }
        if (!isdigit(c))
            return (0);
    }
    if (c == '0' && *(p + 1) == 'x') 
    {
        p += 2;
        c = *p;
        n = xtod(c);
        while ((c = *++p) && isxdigit(c)) 
        {
            n *= 16; /* two steps to avoid unnecessary overflow */
            n += xtod(c); /* accum neg to avoid surprises at MAX */
        }
    } 
    else 
    {
        n = '0' - c;
        while ((c = *++p) && isdigit(c)) 
        {
            n *= 10; /* two steps to avoid unnecessary overflow */
            n += '0' - c; /* accum neg to avoid surprises at MAX */
        }
    }
    return (neg ? n : -n);
}


int substring(char **ptr, char *string, char *pattern)
{   
    register int i;
    

    ptr[0]=(char *)strtok(string, pattern);
    for (i=0;ptr[i]!=NULL;++i)
    {   
    	ptr[i+1]=strtok(NULL,pattern);
    }
    return i;
}




// --------------------------------------------------------------------
//		queue maintain function
// --------------------------------------------------------------------
inline void Queue_Init(Queue *q, int queue_size)
{
	q->front = q->rear = 0;
	q->size = queue_size;
}


inline int Queue_IsEmpty(Queue *q)
{
    return (q->rear == q->front);
}

inline int Queue_IsFull(Queue *q)
{
    return ((q->front+1)%q->size == q->rear);
}

inline int Queue_GetLength(Queue *q)
{
	return (q->front + q->size - q->rear)%q->size;
}

inline int Queue_GetRemain(Queue *q)
{
	return q->size - Queue_GetLength(q);
}


inline int Queue_Insert(Queue *q, TS_EVENT item)
{
    q->event_buf[q->front++]=item;
    q->front%=q->size;
    
    return 1;			/* success */
}

inline TS_EVENT Queue_Remove(Queue *q)
{
    TS_EVENT item;
    
    item=q->event_buf[q->rear++];
    q->rear%=q->size;
    
    return item;
}


#ifdef PS2_VERSION

// related
void add_mouse_event(cpe_ts_t* ts, int new_x, int new_y)
{
	TS_EVENT item;
	int delta_x;
	int delta_y;
	signed char dx;
	signed char dy;

	delta_x = new_x - ts->cur_point.x;
	delta_y = ts->cur_point.y - new_y;			// 不知為何, y 的值要顛倒才會正確

	for (; delta_x != 0 || delta_y !=0; )
	{
		if (delta_x<=-128)
		{
			dx = -128;
		}
		else if (delta_x>=127)
		{
			dx = 127;
		}
		else
		{
			dx = delta_x;
		}
		delta_x -= dx;
		
		if (delta_y<=-128)
		{
			dy = -128;
		}
		else if (delta_y>=127)
		{
			dy = 127;
		}
		else
		{
			dy = delta_y;
		}
		delta_y -= dy;
		printk("dx = %d, dy=%d\n", dx, dy);

		if (!Queue_IsFull(&ts->event_queue))
		{		
			item.flags = 0x8;
			item.x = dx;
			item.y = dy;
			Queue_Insert(&ts->event_queue, item);
		}
	}

}

#else

void add_mouse_event(cpe_ts_t* ts, int new_x, int new_y)
{
	TS_EVENT item;

	if (!Queue_IsFull(&ts->event_queue))
	{		
		item.flags = ts->mouse_flags;
		item.x = new_x;
		item.y = new_y;
		Queue_Insert(&ts->event_queue, item);
	}
}

#endif


static inline void set_cal(cpe_ts_t* ts, Point *left, Point *right, Point *left_raw, Point *right_raw)
{
	if ( (right->x == left->x) || (right->y == left->y) || 
	     (left_raw->x == right_raw->x) || (left_raw->y == right_raw->y) )
	{
		return;						// 傳入值有問題
	}
	ts->cal.x_u = right->x - left->x;
	ts->cal.x_l = right_raw->x - left_raw->x;

	ts->cal.y_u = right->y - left->y;
	ts->cal.y_l = right_raw->y - left_raw->y;

	ts->cal.left.x = left->x;
	ts->cal.left.y = left->y;
	ts->cal.left_raw.x = left_raw->x;
	ts->cal.left_raw.y = left_raw->y;
	ts->cal.right.x = right->x;
	ts->cal.right.y = right->y;
	ts->cal.right_raw.x = right_raw->x;
	ts->cal.right_raw.y = right_raw->y;
}


/*
 * This is a bottom-half handler that is scheduled after
 * raw X,Y,Z1,Z2 coordinates have been acquired, and does
 * the following:
 *
 *   - computes touch screen pressure resistance
 *   - if pressure is above a threshold considered to be pen-down:
 *         - compute calibrated X and Y coordinates
 *         - queue a new TS_EVENT
 *         - signal asynchronously and wake up any read
 */
static void chug_raw_data(cpe_ts_t* ts)
{
	int new_x;
	int new_y;
	unsigned long flags;

	// user touch 的 (x, y) 位置
	new_x = (ts->raw_point.x - ts->cal.left_raw.x) * ts->cal.x_u / ts->cal.x_l + ts->cal.left.x;
	new_y = (ts->raw_point.y - ts->cal.left_raw.y) * ts->cal.y_u / ts->cal.y_l + ts->cal.left.y;

	spin_lock_irqsave(&ts->lock, flags);
	add_mouse_event(ts, new_x, new_y);
	ts->cur_point.x = new_x;
	ts->cur_point.y = new_y;
	spin_unlock_irqrestore(&ts->lock, flags);
	

	// async notify
	if (ts->fasync)
	{
		kill_fasync(&ts->fasync, SIGIO, POLL_IN);
	}
	
	// wake up any read call
	if (waitqueue_active(&ts->wait))
	{
		wake_up_interruptible(&ts->wait);
	}
}


//read +x value
//read +y value
void DFR_high_resolution(cpe_ts_t *ts,int *x, int *y)
{
	unsigned int ssp_status;
	int data_size;

    *(volatile unsigned int *)(ts->io_base+SSP_CONTROL2)|=(SSP_TXFCLR|SSP_RXFCLR);
    *(volatile unsigned char *)(ts->io_base + SSP_DATA)=X_DFR12;
    *(volatile unsigned char *)(ts->io_base + SSP_DATA)=Y_DFR12;
	while(*(volatile unsigned int *)(ts->io_base+SSP_STATUS)&SSP_BUSY)
	    ;
    ssp_status = *(volatile unsigned int *)(ts->io_base + SSP_STATUS);
    data_size = (ssp_status & SSP_RFVE) >> 4;
    if (data_size != 2)
    {
    	*(volatile unsigned int *)(ts->io_base+SSP_CONTROL2)|=(SSP_TXFCLR|SSP_RXFCLR);
		*x = *y = 100000;						// 隨便不合法的值
		return;
    }
	
	*y = 0xFFF - (*(volatile unsigned short *)(ts->io_base+SSP_DATA) & 0xFFF);	// x, y values exchange
	*x = *(volatile unsigned short *)(ts->io_base+SSP_DATA) & 0xFFF;	// x, y values exchange
	mdelay(10);
	//printk("(%d,%d)",*x,*y);
}

#define TRY_CNT		15						// 假設在 try 了 TRY_CNT 次之內, 讀出的 x, y 會穩定
int  GetTouchPanelPos(cpe_ts_t *ts)
{
    int x=0,y=0;
	int dx, dy;
	int x1, y1;
	int x2, y2;
	int i;
	
	*(volatile unsigned int *)(ts->io_base+SSP_CONTROL2)|=(SSP_TXFCLR|SSP_RXFCLR);
	
	for (i=0; i<TRY_CNT; ++i)
	{
		DFR_high_resolution(ts,&x1,&y1);
		if (x1 == 100000) 
		    continue; // add by Charles Tsai
		DFR_high_resolution(ts,&x2,&y2);
		if (x1 == 100000) 
		    continue; // add by Charles tsai
        if((x1==0)||(y1==0)||(x2==0)||(y2==0))
            break;
		dx = x1 - x2;
		if(dx < 0)
		    dx = -dx;
		dy = y1 - y2;
		if(dy < 0)
		    dy = -dy;
	    if(i==0)
	    {
	       x=(x1+x2)/2;
	       y=(y1+y2)/2;
	    }
	    else
	    {
            x=(x+x1+x2)/3;
            y=(y+y1+y2)/3;
        }
		if ( !((dx > 0x25) | (dy > 0x25)) ) // org 40 
			break;
		mdelay(1);
	}
	if((x==0)||(y==0))
	    return 0;
//printk("[%d,%d]\n",x,y);	
	 /*printk("got (x, y) = (%d, %d)\n", (x1+x2)/2, (y1+y2)/2);*/
	if (i != TRY_CNT)
	{
#if 0
		ts->raw_point.x = (x1+x2)/2;
		ts->raw_point.y = (y1+y2)/2;
#else
        ts->raw_point.x = x;
		ts->raw_point.y = y;
#endif
		ts->last_jiffies = jiffies;
	}
	else{
		return 0; // add by Charles Tsai
	}
	return 1; // add by charles Tsai
}



void touch_release_timeout(unsigned long ptr)
{
	cpe_ts_t *ts = (cpe_ts_t*)ptr;
	
	ts->mouse_flags = MOUSE_RELEASE;
	chug_raw_data(ts);
}



void touch_debounce_timeout(unsigned long ptr)
{
	cpe_ts_t *ts = (cpe_ts_t*)ptr;

	ts->mouse_flags = MOUSE_PRESS;
	chug_raw_data(ts);

	mod_timer(&ts->release_timer, jiffies + RELEASE_TIME);	
	enable_irq(ts->irq);
}



static void touch_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	int ret; // add by Charles Tsai
//	unsigned int flags;
	cpe_ts_t *ts = (cpe_ts_t*)dev_id;

	spin_lock(&ts->lock);
	ret = GetTouchPanelPos(ts); // modify by Charles Tsai
    if (ret == 0)
	{
        spin_unlock(&ts->lock);
        return;
	}
	ts->debounce_timer.expires = jiffies + DEBOUNCE_TIME;
	add_timer(&ts->debounce_timer);
	disable_irq(irq);
	spin_unlock(&ts->lock);
}


/* +++++++++++++ File operations ++++++++++++++*/

static int
cpets_fasync(int fd, struct file *filp, int mode)
{
	int retval;
	cpe_ts_t* ts = (cpe_ts_t*)filp->private_data;
	
	retval = fasync_helper(fd, filp, mode, &ts->fasync);
	if (retval < 0)
	{
		return retval;
	}

	return 0;
}



static unsigned int
cpets_poll(struct file * filp, poll_table * wait)
{
	cpe_ts_t* ts = (cpe_ts_t*)filp->private_data;
	
	poll_wait(filp, &ts->wait, wait);
	if (!Queue_IsEmpty(&ts->event_queue))
	{
		return POLLIN | POLLRDNORM;
	}

	return 0;
}


static ssize_t
cpets_read(struct file * filp, char * buf, size_t count, loff_t * l)
{
	cpe_ts_t* ts = (cpe_ts_t*)filp->private_data;
	unsigned long flags;
	TS_EVENT event;
	int i;

	while (Queue_IsEmpty(&ts->event_queue))
	{
		if (filp->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		interruptible_sleep_on(&ts->wait);
		if (signal_pending(current))
		{
			return -ERESTARTSYS;
		}
	}

#ifdef PS2_VERSION
	for (i=count; i>=sizeof(TS_EVENT); i-=3, buf+=3)
#else
	for (i=count; i>=sizeof(TS_EVENT); i-=sizeof(TS_EVENT), buf+=sizeof(TS_EVENT))
#endif
	{
		if (Queue_IsEmpty(&ts->event_queue))
		{
			break;
		}
		spin_lock_irqsave(&ts->lock, flags);
		
		event = Queue_Remove(&ts->event_queue);
		
		spin_unlock_irqrestore(&ts->lock, flags);
#ifdef PS2_VERSION
		copy_to_user(buf, &event, 3);
#else
		copy_to_user(buf, &event, sizeof(TS_EVENT));
#endif
	}

	return count - i;
}


static int
cpets_open(struct inode * inode, struct file * filp)
{
	cpe_ts_t* ts;
	unsigned long flags;

	filp->private_data = ts = &cpe_ts;


	spin_lock_irqsave(&ts->lock, flags);

#ifdef not_complete_yet
	fLib_SSPClearTxFIFO(VA_CPE_SSP1_BASE);
	fLib_SSPClearRxFIFO(VA_CPE_SSP1_BASE);


	/*
	 * init bh handler that chugs the raw data (calibrates and
	 * calculates touch pressure).
	 */
	ts->chug_tq.routine = chug_raw_data;
	ts->chug_tq.data = ts;
	ts->cur_point.x = 320;
	ts->cur_point.y = 240;
#endif /* end_of_not */

	// flush event queue
	Queue_Init(&ts->event_queue, EVENT_BUFSIZE);

	spin_unlock_irqrestore(&ts->lock, flags);


	MOD_INC_USE_COUNT;
	return 0;
}

static int
cpets_release(struct inode * inode, struct file * filp)
{
//	cpe_ts_t* ts = (cpe_ts_t*)filp->private_data;
	cpets_fasync(-1, filp, 0);
	printk("enter au100_release\n");

	MOD_DEC_USE_COUNT;
	return 0;
}


static struct file_operations ts_fops = {
  	owner:      THIS_MODULE,
	read:       cpets_read,
	poll:       cpets_poll,
	fasync:     cpets_fasync,
	open:		cpets_open,
	release:	cpets_release,
};


/*
 * miscdevice structure for misc driver registration.
 */
static struct miscdevice cpe_ts_dev = {
	minor: TS_MINOR, 
	name: "CPE touch screen", 
	fops: &ts_fops,
};


static void init_touch(cpe_ts_t *ts) 
{
//	int i;
	
	outl( (2<<12)|		// frame format
	      (1<<5) |		// frame/sync polarity
	      (3<<2) | 		// operation mode
	      (0)	 ,		// SCLK phase
	      ts->io_base + SSP_CONTROL0);
	      
	outl( (8<<24) |		// PDL
		  (11<<16)|		// SDL
		  (10), 		// SCLKDIV
		  ts->io_base + SSP_CONTROL1);
	
	*(volatile unsigned int *)(ts->io_base+SSP_CONTROL2)=(SSP_SSPEN|SSP_TXDOE);
    *(volatile unsigned int *)(ts->io_base+SSP_CONTROL2)|=(SSP_TXFCLR|SSP_RXFCLR);
}

static struct proc_dir_entry *proc_ts;

void do_parse(char *buffer)
{
	cpe_ts_t* ts = &cpe_ts;
	int argc;
	char *argv[20];
	static int point_idx;				// 傳進來的是第幾個 point 
	Point new_point;
	Point new_raw_point;

	// printk("got setting: %s\n", buffer);
	argc = substring(argv, buffer, " (,)");
	if (argc == 5 && strcmp(argv[0], "point")==0)
	{
		new_point.x = atoi(argv[1]);
		new_point.y = atoi(argv[2]);
		new_raw_point.x = atoi(argv[3]);
		new_raw_point.y = atoi(argv[4]);

		if (point_idx%2==0)
		{
			set_cal(ts, &new_point, &ts->cal.right, &new_raw_point, &ts->cal.right_raw);
		}
		else
		{
			set_cal(ts, &ts->cal.left, &new_point, &ts->cal.left_raw, &new_raw_point);
		}
		++point_idx;
	}
}


static int ts_read_proc(char *page, char **start,  off_t off, int count, int *eof, void *data)
{
	cpe_ts_t* ts = &cpe_ts;
	int num;
	
	num = sprintf(page, "raw (%d, %d)\n", ts->raw_point.x, ts->raw_point.y);
	num += sprintf(page+num, "left (%d, %d)\n", ts->cal.left.x, ts->cal.left.y);
	num += sprintf(page+num, "left_raw (%d, %d)\n", ts->cal.left_raw.x, ts->cal.left_raw.y);
	num += sprintf(page+num, "right (%d, %d)\n", ts->cal.right.x, ts->cal.right.y);
	num += sprintf(page+num, "right_raw (%d, %d)\n", ts->cal.right_raw.x, ts->cal.right_raw.y);
	
	return num;
}


static int ts_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char *page;
	char *lines[256];
	int line_num;
	int i;
    
    
    if (count > PAGE_SIZE)
    {
		return -EOVERFLOW;
	}
 
	if (!(page = (char *) __get_free_page(GFP_KERNEL)))
	{
		return -ENOMEM;
	}
	
	copy_from_user(page, buffer, count);
	page[count] = '\0';
 	line_num = substring(lines, page, "\r\n");

 	for (i=0; i<line_num; ++i)
 	{
		do_parse(lines[i]);
 	}

	free_page((ulong) page);
	

	return count;
}


int __init cpets_init_module(void)
{
	cpe_ts_t    *ts = &cpe_ts;
	int         ret;
	Point       left, right, left_raw, right_raw;

	if ((ret = misc_register(&cpe_ts_dev))<0)
	{
		err("can't get major number");
		return ret;
	}
	info("Faraday SSP Touch Panel Registered");

	// --------------------------------------------------------------------
	//		初始化 ts 資料結構
	// --------------------------------------------------------------------
	memset(ts, 0, sizeof(cpe_ts_t));
	init_waitqueue_head(&ts->wait);
	spin_lock_init(&ts->lock);

	// initial calibration values
	left.x = 0;
	left.y = 0;
	left_raw.x = 0;
	left_raw.y = 0;
	
	/*right.x = 640;
	right.y = 480;*/
	right.x = TOUCH_PANEL_RIGHT_X;
	right.y = TOUCH_PANEL_RIGHT_Y;
	right_raw.x = 4096; /* org 4800 */
	right_raw.y = 4096; /* org 4800 */
	set_cal(ts, &left, &right, &left_raw, &right_raw);
	
	Queue_Init(&ts->event_queue, EVENT_BUFSIZE);

	/*ts->cur_point.x = 320;
	ts->cur_point.y = 240;*/
    ts->cur_point.x = TOUCH_PANEL_RIGHT_X/2;
	ts->cur_point.y = TOUCH_PANEL_RIGHT_Y/2;
	ts->irq = IRQ_TOUCH;
	ts->io_base = CPE_SSP1_VA_BASE;
	
	init_timer(&ts->debounce_timer);
    ts->debounce_timer.function = touch_debounce_timeout;
	ts->debounce_timer.data = (unsigned long)ts;
	
	init_timer(&ts->release_timer);
	ts->release_timer.function = touch_release_timeout;
	ts->release_timer.data = (unsigned long)ts;
	
	init_touch(ts);

	cpe_int_set_irq(ts->irq,LEVEL, L_ACTIVE);
	if ((ret = request_irq(ts->irq, touch_interrupt_handler, SA_INTERRUPT, TS_NAME, ts)))
	{
		err("could not get IRQ");
		return ret;
	}

	if ((proc_ts = create_proc_entry( "ts", 0, 0 )))
	{
		proc_ts->read_proc = ts_read_proc;
		proc_ts->write_proc = ts_write_proc;
		proc_ts->owner = THIS_MODULE;
	}
	return 0;

}

void
cpets_cleanup_module(void)
{
	cpe_ts_t* ts = &cpe_ts;
	if (proc_ts)
	{
    	remove_proc_entry( "ts", 0);
    }
	free_irq(ts->irq, &cpe_ts);
	misc_deregister(&cpe_ts_dev);
}

/* Module information */
MODULE_AUTHOR("www.faraday.com");
MODULE_DESCRIPTION("cpe/ADS7846 Touch Screen Driver");

module_init(cpets_init_module);
module_exit(cpets_cleanup_module);
