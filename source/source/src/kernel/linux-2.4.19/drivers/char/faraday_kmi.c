// --------------------------------------------------------------------
//	2002-12-26: lmc83
//		- based on pc_keyb.c
// --------------------------------------------------------------------
// modified by ivan 2004/11/4 01:48pm

#include <linux/config.h>

#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/signal.h>
#include <linux/init.h>
#include <linux/kbd_ll.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/kbd_kern.h>
#include <linux/vt_kern.h>
#include <linux/smp_lock.h>
#include <linux/kd.h>
#include <linux/pm.h>
#include <asm/keyboard.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/arch/cpe/cpe.h>
#include <asm/arch/cpe_int.h>   //Interrupt definition
//#include <asm/arch/kmi.h> 
#include "faraday_kmi.h"

#define DEV_NAME			"Faraday KBC"

//static spinlock_t kbd_controller_lock = SPIN_LOCK_UNLOCKED;
static unsigned char handle_kbd_event(void);
static volatile unsigned char reply_expected;
static volatile unsigned char acknowledge;
static volatile unsigned char resend;
int         cango = 0;
FaradayLEDS fa_led_state;
/* end add */

#define KMI_TIMEOUT				250					// 等待 kmi 可以開始傳送的時間
#define READ_LOOPS				10					// 等待 kmi 有資料的時間 (minisecond)
#define WRITE_LOOPS				10					// 等待 kmi 真的將資料傳送出去的時間 (minisecond)
#define KBD_LOOPS				6


// --------------------------------------------------------------------
//		kmi function
//	input:
//		base: CPE_KBD_BASE ==> for keyboard
//			  CPE_MOUSE_BASE ==> for mouse
// --------------------------------------------------------------------
/* add by Charles Tsai */
#if 0
static void Force_Clock_Low(u32 base, u32 enable)
{
	u32 data;

	data = inl(base+ KMI_Control);
	if (enable){
		data |= Force_clock_line_low;
	}
	else{
		data &= (~Force_clock_line_low);
	}
	outl(data, base+ KMI_Control);
}
#endif
/* enda dd */
static  void clear_Rx_Int(u32 base)
{
	//printk("+clear_Rx_Int\n");	
    outl( inl(base + KMI_Control)|Clear_RX_INT, base + KMI_Control);
}


static  void clear_Tx_Int(u32 base)
{     
	//printk("+clear_Tx_Int\n");		
    outl( inl(base + KMI_Control)|Clear_TX_INT, base + KMI_Control);
}


static  void Kmi_Init(u32 base)
{
	printk("init KMI Base addr = 0x%08x\n", base);

	//john modify for Mouse
	if (base == CPE_KBD_VA_BASE)
       outl( 0x58, base + KMI_RequestToSend );
	else
       outl( 0xA50, base + KMI_RequestToSend );

    outl( Clear_RX_INT | Clear_TX_INT | Enable_RX_INT | Enable_TX_INT | Enable_KMI, base + KMI_Control ); //0xdc
}


static  int Kmi_Read_Status(u32 base)
{	
	return inl(base + KMI_Status);
}


// --------------------------------------------------------------------
//		從 kmi 讀出一個 long (沒 check 是否有資料)
// --------------------------------------------------------------------
static  int Kmi_Read_Input(u32 base)
{
	int val;
	
	//printk("+Kmi_Read_Input\n");	

	val = inl(base + KMI_Receive);
	clear_Rx_Int(base);
	
	return val;
}


#define KBD_NO_DATA		(-1)	/* No data */
#define KBD_BAD_DATA	(-2)	/* Parity or other error */


// --------------------------------------------------------------------
//		從 kmi 讀出一個 long (有 check 是否有資料) (for loop READ_LOOPS 等待是否有資料)
// --------------------------------------------------------------------
//static int __init Kmi_Read_Data(u32 base)
static int Kmi_Read_Data(u32 base)
{
	int retval=KBD_NO_DATA;
	unsigned int status;
	//unsigned char status;
	unsigned long loops = jiffies + READ_LOOPS*HZ/10;
	
	//printk("+Kmi_Read_Data\n");

	///for (; loops>0; --loops)
	for (; jiffies<loops; )
	{
		status = Kmi_Read_Status(base);
		if (status & RX_Full)
		{
			retval = Kmi_Read_Input(base);
		
			///printk("Kmi_Read_Data: %x\n", retval);
			return retval;
		}
		///mdelay(1);
	}
	//printk("not RX_Full\n");
	return KBD_NO_DATA;

}


// --------------------------------------------------------------------
//		等待 kmi 可以傳送資料給 keyboard
// --------------------------------------------------------------------
static void Kmi_Wait(u32 base)
{
	unsigned long timeout = KMI_TIMEOUT;

	//printk("+Kmi_Wait\n");
	do {
		/*
		 * "handle_kbd_event()" will handle any incoming events
		 * while we wait - keypresses or mouse movement.
		 */
		///unsigned char status = handle_km_event(base);
		//unsigned char status = Kmi_Read_Status(base);		/// 暫時用 Kmi_Read_Status 代替
		unsigned int status = Kmi_Read_Status(base);		/// 暫時用 Kmi_Read_Status 代替

		if ( (status & TX_Empty) )
			return;
		
		mdelay(1);
		timeout--;
	} while (timeout);
#ifdef KBD_REPORT_TIMEOUTS
	printk(KERN_WARNING "Keyboard timed out[1]\n");
#endif
}


// --------------------------------------------------------------------
//		寫一個 long 到 kmi
// --------------------------------------------------------------------
static void Kmi_Write_Data(u32 base, unsigned char data)
{
	volatile u32 int_status;
	unsigned long loops = jiffies + WRITE_LOOPS*HZ/10;
	
	//printk("+Kmi_Write_Data..= 0x%x\n",data);

	Kmi_Wait(base);
	outl( data, base + KMI_Transmit );
	///for (; try_cnt>0; --try_cnt)
	for (; jiffies<loops; )
	{
		int_status = inl(base + KMI_IntStatus);

		if ( (int_status & KEYBOARD_TXINT) != 0)
		{
			clear_Tx_Int(base);
			break;
		}
		///mdelay(1);
	}
	///if (try_cnt==0)
	if (jiffies >= loops)
		printk("write failure\n");
}


static  int Kmi_Write_Data_Wait_Ack(u32 base, unsigned char data)
{
	//printk("+Kmi_Write_Data_Wait_Ack\n");

	Kmi_Write_Data(base, data);
	return Kmi_Read_Data(base);
}


//static void __init Kmi_Clear_Input(u32 base)
static void Kmi_Clear_Input(u32 base)
{
	int maxread = 100;	/* Random number */

	//printk("+Kmi_Clear_Input\n");
	do {
		if (Kmi_Read_Data(base) == KBD_NO_DATA)
			break;
	} while (--maxread);
}


/*
 * Translation of escaped scancodes to keycodes.
 * This is now user-settable.
 * The keycodes 1-88,96-111,119 are fairly standard, and
 * should probably not be changed - changing might confuse X.
 * X also interprets scancode 0x5d (KEY_Begin).
 *
 * For 1-88 keycode equals scancode.
 */

#define E0_KPENTER 96
#define E0_RCTRL   97
#define E0_KPSLASH 98
#define E0_PRSCR   99
#define E0_RALT    100
#define E0_BREAK   101  /* (control-pause) */
#define E0_HOME    102
#define E0_UP      103
#define E0_PGUP    104
#define E0_LEFT    105
#define E0_RIGHT   106
#define E0_END     107
#define E0_DOWN    108
#define E0_PGDN    109
#define E0_INS     110
#define E0_DEL     111

#define E1_PAUSE   119

/*
 * The keycodes below are randomly located in 89-95,112-118,120-127.
 * They could be thrown away (and all occurrences below replaced by 0),
 * but that would force many users to use the `setkeycodes' utility, where
 * they needed not before. It does not matter that there are duplicates, as
 * long as no duplication occurs for any single keyboard.
 */
#define SC_LIM 89

#define FOCUS_PF1 85           /* actual code! */
#define FOCUS_PF2 89
#define FOCUS_PF3 90
#define FOCUS_PF4 91
#define FOCUS_PF5 92
#define FOCUS_PF6 93
#define FOCUS_PF7 94
#define FOCUS_PF8 95
#define FOCUS_PF9 120
#define FOCUS_PF10 121
#define FOCUS_PF11 122
#define FOCUS_PF12 123

#define JAP_86     124
/* tfj@olivia.ping.dk:
 * The four keys are located over the numeric keypad, and are
 * labelled A1-A4. It's an rc930 keyboard, from
 * Regnecentralen/RC International, Now ICL.
 * Scancodes: 59, 5a, 5b, 5c.
 */
#define RGN1 124
#define RGN2 125
#define RGN3 126
#define RGN4 127

static unsigned char high_keys[128 - SC_LIM] = {
  RGN1, RGN2, RGN3, RGN4, 0, 0, 0,                   /* 0x59-0x5f */
  0, 0, 0, 0, 0, 0, 0, 0,                            /* 0x60-0x67 */
  0, 0, 0, 0, 0, FOCUS_PF11, 0, FOCUS_PF12,          /* 0x68-0x6f */
  0, 0, 0, FOCUS_PF2, FOCUS_PF9, 0, 0, FOCUS_PF3,    /* 0x70-0x77 */
  FOCUS_PF4, FOCUS_PF5, FOCUS_PF6, FOCUS_PF7,        /* 0x78-0x7b */
  FOCUS_PF8, JAP_86, FOCUS_PF10, 0                   /* 0x7c-0x7f */
};

/* BTC */
#define E0_MACRO   112
/* LK450 */
#define E0_F13     113
#define E0_F14     114
#define E0_HELP    115
#define E0_DO      116
#define E0_F17     117
#define E0_KPMINPLUS 118
/*
 * My OmniKey generates e0 4c for  the "OMNI" key and the
 * right alt key does nada. [kkoller@nyx10.cs.du.edu]
 */
#define E0_OK	124
/*
 * New microsoft keyboard is rumoured to have
 * e0 5b (left window button), e0 5c (right window button),
 * e0 5d (menu button). [or: LBANNER, RBANNER, RMENU]
 * [or: Windows_L, Windows_R, TaskMan]
 */
#define E0_MSLW	125
#define E0_MSRW	126
#define E0_MSTM	127

static unsigned char e0_keys[128] = {
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x00-0x07 */
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x08-0x0f */
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x10-0x17 */
  0, 0, 0, 0, E0_KPENTER, E0_RCTRL, 0, 0,	      /* 0x18-0x1f */
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x20-0x27 */
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x28-0x2f */
  0, 0, 0, 0, 0, E0_KPSLASH, 0, E0_PRSCR,	      /* 0x30-0x37 */
  E0_RALT, 0, 0, 0, 0, E0_F13, E0_F14, E0_HELP,	      /* 0x38-0x3f */
  E0_DO, E0_F17, 0, 0, 0, 0, E0_BREAK, E0_HOME,	      /* 0x40-0x47 */
  E0_UP, E0_PGUP, 0, E0_LEFT, E0_OK, E0_RIGHT, E0_KPMINPLUS, E0_END,/* 0x48-0x4f */
  E0_DOWN, E0_PGDN, E0_INS, E0_DEL, 0, 0, 0, 0,	      /* 0x50-0x57 */
  0, 0, 0, E0_MSLW, E0_MSRW, E0_MSTM, 0, 0,	      /* 0x58-0x5f */
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x60-0x67 */
  0, 0, 0, 0, 0, 0, 0, E0_MACRO,		      /* 0x68-0x6f */
  0, 0, 0, 0, 0, 0, 0, 0,			      /* 0x70-0x77 */
  0, 0, 0, 0, 0, 0, 0, 0			      /* 0x78-0x7f */
};


int kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
	//printk("+kbd_setkeycode\n");

	if (scancode < SC_LIM || scancode > 255 || keycode > 127)
	  	return -EINVAL;
	
	if (scancode < 128)
	  	high_keys[scancode - SC_LIM] = keycode;
	else
	  	e0_keys[scancode - 128] = keycode;

	return 0;
}

int kbd_getkeycode(unsigned int scancode)
{
	//printk("+kbd_getkeycode\n");

	return
	  (scancode < SC_LIM || scancode > 255) ? -EINVAL :
	  (scancode < 128) ? high_keys[scancode - SC_LIM] :
	    e0_keys[scancode - 128];
}


int kbd_translate(unsigned char scancode, unsigned char *keycode, char raw_mode)
{
	static int prev_scancode;

	//printk("+kbd_translate\n");

	/* special prefix scancodes.. */
	if (scancode == 0xe0 || scancode == 0xe1) {
		prev_scancode = scancode;
		return 0;
	}

	/* 0xFF is sent by a few keyboards, ignore it. 0x00 is error */
	if (scancode == 0x00 || scancode == 0xff) {
		prev_scancode = 0;
		return 0;
	}

	scancode &= 0x7f;

	if (prev_scancode) 
	{
	  	/*
	   	 * usually it will be 0xe0, but a Pause key generates
	     * e1 1d 45 e1 9d c5 when pressed, and nothing when released
	     */
		if (prev_scancode != 0xe0) 
		{
	      	if (prev_scancode == 0xe1 && scancode == 0x1d) 
	      	{
		  		prev_scancode = 0x100;
		  		return 0;
	      	} 
	      	else if (prev_scancode == 0x100 && scancode == 0x45) 
	      	{
		  		*keycode = E1_PAUSE;
		  		prev_scancode = 0;
	      	} 
	      	else 
	      	{
#ifdef KBD_REPORT_UNKN
		  		if (!raw_mode)
		    		printk(KERN_INFO "keyboard: unknown e1 escape sequence\n");
#endif
		  		prev_scancode = 0;
		  		return 0;
	      	}
	  	} 
	  	else 
	  	{
	      	prev_scancode = 0;
      	 	/*
	         *  The keyboard maintains its own internal caps lock and
	         *  num lock statuses. In caps lock mode E0 AA precedes make
	         *  code and E0 2A follows break code. In num lock mode,
	         *  E0 2A precedes make code and E0 AA follows break code.
	         *  We do our own book-keeping, so we will just ignore these.
	         */
	        /*
	         *  For my keyboard there is no caps lock mode, but there are
	         *  both Shift-L and Shift-R modes. The former mode generates
	         *  E0 2A / E0 AA pairs, the latter E0 B6 / E0 36 pairs.
	         *  So, we should also ignore the latter. - aeb@cwi.nl
	         */
	      	if (scancode == 0x2a || scancode == 0x36)
				return 0;

	      	if (e0_keys[scancode])
				*keycode = e0_keys[scancode];
	      	else 
	      	{
#ifdef KBD_REPORT_UNKN
		  		if (!raw_mode)
		    		printk(KERN_INFO "keyboard: unknown scancode e0 %02x\n",
			   		scancode);
#endif
		  		return 0;
	      	}
	  	}
	} 
	else if (scancode >= SC_LIM) 
	{
	    /* This happens with the FOCUS 9000 keyboard
	       Its keys PF1..PF12 are reported to generate
	       55 73 77 78 79 7a 7b 7c 74 7e 6d 6f
	       Moreover, unless repeated, they do not generate
	       key-down events, so we have to zero up_flag below */
	    /* Also, Japanese 86/106 keyboards are reported to
	       generate 0x73 and 0x7d for \ - and \ | respectively. */
	    /* Also, some Brazilian keyboard is reported to produce
	       0x73 and 0x7e for \ ? and KP-dot, respectively. */

	 	*keycode = high_keys[scancode - SC_LIM];

	  	if (!*keycode) 
	  	{
	      	if (!raw_mode) 
	      	{
#ifdef KBD_REPORT_UNKN
		  		printk(KERN_INFO "keyboard: unrecognized scancode (%02x)"
			 		" - ignored\n", scancode);
#endif
	      	}
	      	return 0;
	  	}
 	} 
 	else
 	{
	  	*keycode = scancode;
	}
 	return 1;
}


char kbd_unexpected_up(unsigned char keycode)
{
	//printk("+kbd_unexpected_up\n");

	/* unexpected, but this can happen: maybe this was a key release for a
	   FOCUS 9000 PF key; if we want to see it, we have to clear up_flag */
	if (keycode >= SC_LIM || keycode == 85)
	    return 0;
	else
	    return 0200;
}


static int do_acknowledge(unsigned char scancode)
{
	//printk("+do_acknowledge\n");

	if (reply_expected) 
	{
	  /* Unfortunately, we must recognise these codes only if we know they
	   * are known to be valid (i.e., after sending a command), because there
	   * are some brain-damaged keyboards (yes, FOCUS 9000 again) which have
	   * keys with such codes :(
	   */
		if (scancode == KBD_REPLY_ACK) 
		{
			acknowledge = 1;
			reply_expected = 0;
			return 0;
		} 
		else if (scancode == KBD_REPLY_RESEND) 
		{
			resend = 1;
			reply_expected = 0;
			return 0;
		}
		/* Should not happen... */
#if 0
		printk(KERN_DEBUG "keyboard reply expected - got %02x\n", scancode);
#endif
	}

	return 1;
}


int fkmi_pm_resume(struct pm_dev *dev, pm_request_t rqst, void *data) 
{
	//printk("+fkmi_pm_resume\n");

       return 0;
}


static unsigned char kbd_exists = 1;

static  void handle_keyboard_event(unsigned char scancode)
{
#ifdef CONFIG_VT
	//printk("enter here\n");
	kbd_exists = 1;
	if (do_acknowledge(scancode))
		handle_scancode(scancode, !(scancode & 0x80));
#endif	
	
	//printk("+handle_keyboard_event\n");

	tasklet_schedule(&keyboard_tasklet);
}	

/*
 * This reads the keyboard status port, and does the
 * appropriate action.
 *
 * It requires that we hold the keyboard controller
 * spinlock.
 */
static unsigned char handle_kbd_event(void)
{
	//unsigned char status;
	unsigned int status;
	int loops=KBD_LOOPS;
	
	//printk("+handle_kbd_event\n");

	for (; loops>0; --loops)
	{
		status = Kmi_Read_Status(CPE_KBD_VA_BASE);
		if (status & RX_Full)
		{
			unsigned char scancode;

			scancode = Kmi_Read_Input(CPE_KBD_VA_BASE);
			handle_keyboard_event(scancode);
			break;
		}
		mdelay(1);
	}
	return 1;
}

static void keyboard_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	//printk("+keyboard_interrupt\n");

///	spin_lock_irq(&kbd_controller_lock);
	handle_kbd_event();
///	spin_unlock_irq(&kbd_controller_lock);
}


/*
 * send_data sends a character to the keyboard and waits
 * for an acknowledge, possibly retrying if asked to. Returns
 * the success status.
 *
 * Don't use 'jiffies', so that we don't depend on interrupts
 */
static int send_data(unsigned char data)
{

	int retries = 3;

	//printk("+send_data\n");

	do {
		unsigned long timeout = KBD_TIMEOUT;

		acknowledge = 0; /* Set by interrupt routine on receipt of ACK. */
		resend = 0;
		reply_expected = 1;
		Kmi_Write_Data(CPE_KBD_VA_BASE, data);
		for (;;) {
			if (acknowledge)
				return 1;
			if (resend)
				break;
			mdelay(1);
			if (!--timeout) {
#ifdef KBD_REPORT_TIMEOUTS
				printk(KERN_WARNING "keyboard: Timeout - AT keyboard not present?(%02x)\n", data);
#endif
				return 0;
			}
		}
	} while (retries-- > 0);
#ifdef KBD_REPORT_TIMEOUTS
	printk(KERN_WARNING "keyboard: Too many NACKs -- noisy kbd cable?\n");
#endif

	return 0;
}

void fkmi_leds(unsigned char leds)
{
    printk("set led.........\n");//ivan
#if 0
	ExtMaskIRQ(IRQ_ASIC321_KBC);
	Force_Clock_Low(CPE_KBD_VA_BASE, 1);
//	printk("enter fkmi_leds 1:%d\n", fa_led_state.LEDS);
    Kmi_Write_Data(CPE_KBD_VA_BASE, (unsigned char)KBD_CMD_SET_LEDS);
    Kmi_Write_Data(CPE_KBD_VA_BASE, (unsigned char)fa_led_state.LEDS);
	Force_Clock_Low(CPE_KBD_VA_BASE, 0);
	ExtUnmaskIRQ(IRQ_ASIC321_KBC);
	return;
	
	if (kbd_exists && (!send_data(KBD_CMD_SET_LEDS) || !send_data(leds))) {
		send_data(KBD_CMD_ENABLE);	//* re-enable kbd if any errors 
		kbd_exists = 0;
	}
#endif
}

#define DEFAULT_KEYB_REP_DELAY	250
#define DEFAULT_KEYB_REP_RATE	30	/* cps */

static struct kbd_repeat kbdrate={
	DEFAULT_KEYB_REP_DELAY,
	DEFAULT_KEYB_REP_RATE
};

static unsigned char parse_kbd_rate(struct kbd_repeat *r)
{
	static struct r2v{
		int rate;
		unsigned char val;
	} kbd_rates[]={	{5,0x14},
			{7,0x10},
			{10,0x0c},
			{15,0x08},
			{20,0x04},
			{25,0x02},
			{30,0x00}
	};
	static struct d2v{
		int delay;
		unsigned char val;
	} kbd_delays[]={{250,0},
			{500,1},
			{750,2},
			{1000,3}
	};
	int rate=0,delay=0;

	//printk("+parse_kbd_rate\n");

	if (r != NULL)
	{
		int i,new_rate=30,new_delay=250;
		if (r->rate <= 0)
			r->rate=kbdrate.rate;
		if (r->delay <= 0)
			r->delay=kbdrate.delay;
		for (i=0; i < sizeof(kbd_rates)/sizeof(struct r2v); i++)
			if (kbd_rates[i].rate == r->rate){
				new_rate=kbd_rates[i].rate;
				rate=kbd_rates[i].val;
				break;
			}
		for (i=0; i < sizeof(kbd_delays)/sizeof(struct d2v); i++)
			if (kbd_delays[i].delay == r->delay){
				new_delay=kbd_delays[i].delay;
				delay=kbd_delays[i].val;
				break;
			}
		r->rate=new_rate;
		r->delay=new_delay;
	}
	return (delay << 5) | rate;

}

static int write_kbd_rate(unsigned char r)
{
	//printk("+write_kbd_rate\n");

	if (!send_data(KBD_CMD_SET_RATE) || !send_data(r))
	{
		send_data(KBD_CMD_ENABLE); 	/* re-enable kbd if any errors */
		return 0;
	}
	else
		return 1;
}

static int fkmi_rate(struct kbd_repeat *rep)
{
	//printk("+fkmi_rate\n");

	if (rep == NULL)
		return -EINVAL;
	else{
		unsigned char r=parse_kbd_rate(rep);
		struct kbd_repeat old_rep;
		memcpy(&old_rep,&kbdrate,sizeof(struct kbd_repeat));
		if (write_kbd_rate(r)){
			memcpy(&kbdrate,rep,sizeof(struct kbd_repeat));
			memcpy(rep,&old_rep,sizeof(struct kbd_repeat));
			return 0;
		}
	}

	return -EIO;
}

/*
 * In case we run on a non-x86 hardware we need to initialize both the
 * keyboard controller and the keyboard.  On a x86, the BIOS will
 * already have initialized them.
 *
 * Some x86 BIOSes do not correctly initialize the keyboard, so the
 * "kbd-reset" command line options can be given to force a reset.
 * [Ranger]
 */
#ifdef __i386__
 int kbd_startup_reset __initdata = 0;
#else
 int kbd_startup_reset __initdata = 1;
#endif

/* for "kbd-reset" cmdline param */
//static int __init kbd_reset_setup(char *str)
static int kbd_reset_setup(char *str)
{
	//printk("+kbd_reset_setup\n");

	kbd_startup_reset = 1;
	return 1;
}

__setup("kbd-reset", kbd_reset_setup);

#define KBD_NO_DATA	(-1)	/* No data */
#define KBD_BAD_DATA	(-2)	/* Parity or other error */


//static char * __init keyboard_reset(void)
static char * keyboard_reset(void)
{
	int ack;

	//printk("+keyboard_reset\n");
	/*
	 * Reset keyboard. If the read times out
	 * then the assumption is that no keyboard is
	 * plugged into the machine.
	 * This defaults the keyboard to scan-code set 2.
	 *
	 * Set up to try again if the keyboard asks for RESEND.
	 */
	do 
	{
		ack = Kmi_Write_Data_Wait_Ack(CPE_KBD_VA_BASE, KBD_CMD_RESET);

		if (ack == KBD_REPLY_ACK)
			break;
		
		if (ack != KBD_REPLY_RESEND)
			return "Keyboard reset failed, no ACK";
		
	} while (1);

	if (Kmi_Read_Data(CPE_KBD_VA_BASE) != KBD_REPLY_POR)
		return "Keyboard reset failed, no POR";

	Kmi_Write_Data_Wait_Ack(CPE_KBD_VA_BASE, KBD_CMD_CODE_SET);		/// 應會讀到 ack
	Kmi_Write_Data_Wait_Ack(CPE_KBD_VA_BASE, 0x01);
		
	if (Kmi_Write_Data_Wait_Ack(CPE_KBD_VA_BASE, KBD_CMD_ENABLE) != KBD_REPLY_ACK)
	{
		return "Enable keyboard: no ACK";
	}

	/*
	 * Finally, set the typematic rate to maximum.
	 */
	if (Kmi_Write_Data_Wait_Ack(CPE_KBD_VA_BASE, KBD_CMD_SET_RATE) != KBD_REPLY_ACK)
		return "Set rate: no ACK";
	
	if (Kmi_Write_Data_Wait_Ack(CPE_KBD_VA_BASE, 0x00) != KBD_REPLY_ACK)
		return "Set rate: no 2nd ACK";

	return NULL;
}



//void __init keyboard_init(void)
void keyboard_init(void)
{

	//printk("+keyboard_init\n");

	kbd_request_region();

	Kmi_Init(CPE_KBD_VA_BASE);

	if (kbd_startup_reset) 
	{
		char *msg = keyboard_reset();
		if (msg)
			printk(KERN_WARNING "initialize_kbd: %s\n", msg);
	}
	kbd_rate = fkmi_rate;
	
	/* Ok, finally allocate the IRQ, and off we go.. */
	cpe_int_set_irq(IRQ_A321_KBD, EDGE, H_ACTIVE);
	request_irq(IRQ_A321_KBD, keyboard_interrupt, SA_INTERRUPT, DEV_NAME, 0);

    //k_leds = fkmi_leds;	
	Kmi_Clear_Input(CPE_KBD_VA_BASE);				/* Flush any pending input. */
	/* add by Charles Tsai */
	fa_led_state.LEDS = 0;
	fa_led_state.LEDSOnOff[0] = 0;
	fa_led_state.LEDSOnOff[1] = 0;
	fa_led_state.LEDSOnOff[2] = 0;
	fa_led_state.LEDSState[0] = 0;
	fa_led_state.LEDSState[1] = 0;
	fa_led_state.LEDSState[2] = 0;
	fa_led_state.update = 0;
	/* end add */
	cango = 1;
	printk("Faraday keyboard driver installed\n");
}




static int blink_frequency = HZ/2;

/* Tell the user who may be running in X and not see the console that we have 
   panic'ed. This is to distingush panics from "real" lockups. 
   Could in theory send the panic message as morse, but that is left as an
   exercise for the reader.  */ 
void panic_blink(void)
{ 
}  

//static int __init panicblink_setup(char *str)
static int panicblink_setup(char *str)
{
    int par;

	//printk("+panicblink_setup\n");

    if (get_option(&str,&par)) 
	    blink_frequency = par*(1000/HZ);
    return 1;
}

/* panicblink=0 disables the blinking as it caused problems with some console
   switches. otherwise argument is ms of a blink period. */
__setup("panicblink=", panicblink_setup);



/// --------------------------------------------------------------------
///		PS/2 Auxiliary Device
/// --------------------------------------------------------------------

static struct aux_queue *queue;	/* Mouse data buffer. */
static int aux_count = 0;
/* used when we send commands to the mouse that expect an ACK. */
static unsigned char mouse_reply_expected = 0;

#define MAX_RETRIES	60		/* some aux operations take long time*/



static void handle_kmi_mouse_event(unsigned long scancode);

DECLARE_TASKLET_DISABLED(mouse_tasklet, handle_kmi_mouse_event, 0);


/*
 * Send a byte to the mouse.
 */
static void aux_write_dev(int val)
{
	//printk("+aux_write_dev\n");

	//Kmi_Write_Data(CPE_MOUSE_BASE, val);
	Kmi_Write_Data(CPE_MOUSE_VA_BASE, val);
}

/*
 * Send a byte to the mouse & handle returned ack
 */
// --------------------------------------------------------------------
//		ack 由 interrupt handler 處理
// --------------------------------------------------------------------
static void aux_write_ack(int val)
{
	//printk("+aux_write_ack\n");

	Kmi_Wait(CPE_MOUSE_VA_BASE);
	Kmi_Write_Data(CPE_MOUSE_VA_BASE, val);

	/* we expect an ACK in response. */
	mouse_reply_expected++;
}


// --------------------------------------------------------------------
//		ack 由 busy waiting 處理 (用在還沒 enable interrupt)
//	return
//		true ==> 有收到 ack
//		false ==> 沒收到 ack
// --------------------------------------------------------------------
static int aux_write_wait_ack(int val)
{
	//printk("+aux_write_wait_ack\n");

	Kmi_Wait(CPE_MOUSE_VA_BASE);
	Kmi_Write_Data(CPE_MOUSE_VA_BASE, val);
	return (Kmi_Read_Data(CPE_MOUSE_VA_BASE) == AUX_ACK);
}


static unsigned char get_from_queue(void)
{
	unsigned char result;

	//printk("+get_from_queue\n");

	result = queue->buf[queue->tail];
	queue->tail = (queue->tail + 1) & (AUX_BUF_SIZE-1);

	return result;
}


static void handle_mouse_event(unsigned long scancode)
{
	static unsigned char prev_code;
	
	//printk("+handle_mouse_event\n");

	if (mouse_reply_expected) 
	{
		if (scancode == AUX_ACK) 
		{
			//printk("go ack\n");
			mouse_reply_expected--;
			return;
		}
		mouse_reply_expected = 0;
	}

	prev_code = scancode;
	/// add_mouse_randomness(scancode);
	if (aux_count) 
	{
		int head = queue->head;

		queue->buf[head] = scancode;
		head = (head + 1) & (AUX_BUF_SIZE-1);
		if (head != queue->tail) 
		{
			queue->head = head;
			kill_fasync(&queue->fasync, SIGIO, POLL_IN);
			wake_up_interruptible(&queue->proc_list);
		}
	}
}



// --------------------------------------------------------------------
//		for tasklet 的方法
// --------------------------------------------------------------------
static void handle_kmi_mouse_event(unsigned long scancode)
{
//	int retval;
	//unsigned char status;
	unsigned int status;
	int loops=READ_LOOPS;
	
	//printk("+handle_kmi_mouse_event\n");

	for (; loops>0; )
	{
		status = Kmi_Read_Status(CPE_MOUSE_VA_BASE);
		if (status & RX_Full)
		{
			scancode = Kmi_Read_Input(CPE_MOUSE_VA_BASE);							///scancode = fkmi_read_data();
			handle_mouse_event(scancode);
			continue;
		}
		--loops;
		mdelay(1);
	}
}


static void psaux_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	//printk("+psaux_interrupt\n");

	tasklet_schedule(&mouse_tasklet);
}


static inline int queue_empty(void)
{
	//printk("+queue_empty\n");

	return queue->head == queue->tail;
}

static int fasync_aux(int fd, struct file *filp, int on)
{
	int retval;

	//printk("+fasync_aux\n");

	retval = fasync_helper(fd, filp, on, &queue->fasync);
	if (retval < 0)
	{
		return retval;
	}

	return 0;
}



/*
 * Random magic cookie for the aux device
 */
#define AUX_DEV ((void *)queue)

static int release_aux(struct inode * inode, struct file * file)
{
	//printk("+release_aux\n");

	fasync_aux(-1, file, 0);
	if (--aux_count)
		return 0;
	
	free_irq(IRQ_A321_MOUSE, AUX_DEV);
	tasklet_disable(&mouse_tasklet);

	return 0;
}

/*
 * Install interrupt handler.
 * Enable auxiliary device.
 */

static int open_aux(struct inode * inode, struct file * file)
{
	//printk("+open_aux\n");

	if (aux_count++)
		return 0;

	tasklet_enable(&mouse_tasklet);
	queue->head = queue->tail = 0;		/* Flush input queue */

	cpe_int_set_irq(IRQ_A321_MOUSE, EDGE, H_ACTIVE);
	if (request_irq(IRQ_A321_MOUSE, psaux_interrupt, SA_INTERRUPT, "ps/2 mouse", AUX_DEV))
	{
		aux_count--;
		return -EBUSY;
	}

	Kmi_Clear_Input(CPE_MOUSE_VA_BASE);
	aux_write_ack(AUX_ENABLE_DEV); /* Enable aux device */

	return 0;
}


/*
 * Put bytes from input queue to buffer.
 */

static ssize_t read_aux(struct file * file, char * buffer,
			size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	ssize_t i = count;
	unsigned char c;

	//printk("+read_aux\n");

	if (queue_empty())
	{
		if (file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		add_wait_queue(&queue->proc_list, &wait);
repeat:
		current->state = TASK_INTERRUPTIBLE;
		if (queue_empty() && !signal_pending(current)) 
		{
			schedule();
			goto repeat;
		}
		current->state = TASK_RUNNING;
		remove_wait_queue(&queue->proc_list, &wait);
	}
	
	while (i > 0 && !queue_empty()) 
	{
		c = get_from_queue();
		//printk("got %x\n", c);
		put_user(c, buffer++);
		i--;
	}
	
	if (count-i) 
	{
		file->f_dentry->d_inode->i_atime = CURRENT_TIME;
		return count-i;
	}
	if (signal_pending(current))
	{
		return -ERESTARTSYS;
	}

	return 0;
}

/*
 * Write to the aux device.
 */

static ssize_t write_aux(struct file * file, const char * buffer,
			 size_t count, loff_t *ppos)
{
	ssize_t retval = 0;
	
	// printk("enter write_aux\n");
	if (count) 
	{
		ssize_t written = 0;

		if (count > 32)
			count = 32; /* Limit to 32 bytes. */
		do 
		{
			char c;
			get_user(c, buffer++);
			aux_write_dev(c);
			written++;
		} while (--count);
		
		retval = -EIO;
		if (written) 
		{
			retval = written;
			file->f_dentry->d_inode->i_mtime = CURRENT_TIME;
		}
	}

	return retval;
}

static unsigned int aux_poll(struct file *file, poll_table * wait)
{
	//printk("+aux_poll\n");

	poll_wait(file, &queue->proc_list, wait);
	if (!queue_empty())
	{
		return POLLIN | POLLRDNORM;
	}

	return 0;
}



struct file_operations psaux_fops = {
	read:		read_aux,
	write:		write_aux,
	poll:		aux_poll,
	open:		open_aux,
	release:	release_aux,
	fasync:		fasync_aux,
};



/*
 * Initialize driver.
 */
static struct miscdevice psaux_mouse = {
	PSMOUSE_MINOR, "psaux", &psaux_fops
};


/*
 * Check if this is a dual port controller.
 */
//static int __init detect_auxiliary_port(void)
static int detect_auxiliary_port(void)
{
	int retval = 0;

	//printk("+detect_auxiliary_port\n");

	if (aux_write_wait_ack(AUX_RESET))
	{
		printk(KERN_INFO "Detected PS/2 Mouse.\n");
		retval = 1;
	}
	else {
		printk(KERN_INFO "NO PS/2 Mouse. Detected \n");
	}

	return retval;
}



//int __init psaux_init(void)
int psaux_init(void)
{
	//printk("+psaux_init\n");

    Kmi_Init(CPE_MOUSE_VA_BASE);
	detect_auxiliary_port();

  	if (misc_register(&psaux_mouse))
	{
		return -ENODEV;
	}
	queue = (struct aux_queue *) kmalloc(sizeof(*queue), GFP_KERNEL);
	memset(queue, 0, sizeof(*queue));
	queue->head = queue->tail = 0;
	init_waitqueue_head(&queue->proc_list);

	return 0;
}




//void __init kbd_init_hw(void)
void kbd_init_hw(void)
{
#ifdef CONFIG_KMI_KEYB
	//printk("init Faraday Keyboard\n");
	keyboard_init();
#endif

#ifdef CONFIG_KMI_MOUSE
	//printk("init Faraday Mouse\n");
	psaux_init();
#endif
}
