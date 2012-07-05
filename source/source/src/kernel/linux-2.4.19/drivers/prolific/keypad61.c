/*
 *  Copyright (C) 2006-2008 Prolific Technology Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/kbd_ll.h>
#include <linux/kbd_kern.h>
#include <linux/timer.h>
#include <linux/unistd.h>
#include <asm/irq.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <asm/uaccess.h>
#define DEV_KEYPAD  101 //assigned by no meaning, only avoid to conflict
#define DEV_KEYLED  102

#include <asm/io.h>

#define VERSION     "1.0.8"

/* PL1029 include file */
#include <asm/io.h>
#include <asm/atomic.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <linux/major.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/hardware.h>


/* PL1097 include file
#include <asm/arch-pl1091/pl_symbol_alarm.h>
#include <asm/arch/pl_dma.h>
#include <asm/arch/irqs.h>
#include <asm/pl_reg.h>
#include <asm/system.h>
#include <asm/pl_irq.h>
*/
#define DEBUG 0

#if DEBUG
      #define debug(fmt, arg...)  printk(fmt, ##arg); \
	my_delay(100000);
#else
      #define debug(fmt, arg...)
#endif

#if DEBUG
static void my_delay(int count){
       int i;
       int sum=0;
       for(i=0;i<count;i++){
           sum=sum*i;
       }
}
#endif

#define USING_STOP_BEEP_TIMER 1

//#define GPIO_BASE       0xb94c0000
//#define GPIO_BASE    0x194C0000
//#define GPIO_BASE 	        VIRT_IO_ADDRESS(0x194C0000)
//#define GPIO_BASE			rGPIO_BASE    //system will hang at "Prolific addr driver v0.1..."
#define rGPIO_BASE 		        0xd94c0000
#define GPIO_TIMER_PRESCALER    (rGPIO_BASE + 0x00)
#define GPIO_TIMER_CFG          (rGPIO_BASE + 0x04)
#define GPIO_TIMER_DZ4_7        (rGPIO_BASE + 0x08)
#define GPIO_TIMER_DZ0_3        (rGPIO_BASE + 0x0c)
#define GPIO_TIMER_TCON         (rGPIO_BASE + 0x10)

#define GPIO_TIMER_TLE0B        (rGPIO_BASE + 0x14)
#define GPIO_TIMER_TLE1B        (rGPIO_BASE + 0x16)
#define GPIO_TIMER_TLE2B        (rGPIO_BASE + 0x18)
#define GPIO_TIMER_TLE3B        (rGPIO_BASE + 0x1a)  /* Repeat Number */

#define GPIO_ENB                (rGPIO_BASE + 0x54)
#define GPIO_DO                 (rGPIO_BASE + 0x56)  /* Data Ouput */
#define GPIO_OE                 (rGPIO_BASE + 0x58)  /* 1:Output, 0:Input Selection */
#define GPIO_DRIVE_E2           (rGPIO_BASE + 0x5a)  /* E2, E4, E8 Power Driving */
#define GPIO_DI                 (rGPIO_BASE + 0x5c)  /* Data input */
#define GPIO_DRIVE_E4           (rGPIO_BASE + 0x60)
#define GPIO_DRIVE_E8           (rGPIO_BASE + 0x62)
#define GPIO_PF0_PU             (rGPIO_BASE + 0x64)
#define GPIO_PF0_PD             (rGPIO_BASE + 0x66)
#define GPIO_PF1_SMT            (rGPIO_BASE + 0x68)
#define GPIO_INT_STATUS         (rGPIO_BASE + 0x70)
#define GPIO_KEY_ENB            (rGPIO_BASE + 0x78)  /* Keypad enable */
#define GPIO_KEY_POL            (rGPIO_BASE + 0x7a)  /* Keypad input polarity select 0: rasing edge 1: falling edge */
#define GPIO_KEY_PERIOD         (rGPIO_BASE + 0x7c)  /* Keypad Number from 0 to 255 for Debounce */
#define GPIO_KEY_LEVEL          (rGPIO_BASE + 0x7d)  /* Key Level or edge trigger select 0: level, 1: edge*/
#define GPIO_RSTN_61            (rGPIO_BASE + 0x7e)  /*for PL1061*/
#define RSTN_CIR_61             (1L << (29-16))
#define RSTN_KEY_61             (1L << (30-16))
#define RSTN_PWM_61             (1L << (31-16))

#define GPIO_RSTN_63            (rGPIO_BASE + 0x7f)  /*for PL1063*/
#define RSTN_INT_63             (1L << (28-24))
#define RSTN_SCAN_63            (1L << (30-24))
#define RSTN_KEY_63             (1L << (30-24))
#define RSTN_PWM_63             (1L << (31-24))

#define GPIO_KEY_MASK           (rGPIO_BASE + 0x80)
#define GPIO_KEY_STATUS         (rGPIO_BASE + 0x82)


#define KEYPAD_SENSIBILITY      15      /* 0 sensitive - 15 insensitive */


int ascii_to_scancode(int key);

static struct timer_list auto_repeat_timer, led_flash_slow_timer, led_flash_fast_timer, beep_timer;

static int PIN_MASK;
static int input_pins;

static int input_active_low = 0;
static int auto_pins = 0;
//static int auto_repeat[] = {HZ/2, HZ/5};
static int flash_speed[] = {HZ/2, HZ/15};  /* HZ=100 */

////////add by Jackson
#define MAX_KEY_NUM 16
/*
#ifdef CONFIG_PL1063
   #define MAX_KEY_NUM 8
#else
   #define MAX_KEY_NUM 16
#endif
*/
static int auto_repeat_duration[MAX_KEY_NUM];
static int auto_repeat_period[MAX_KEY_NUM];
static int g_auto_repeat_index=0;
static int g_key_up_code[MAX_KEY_NUM];
static int g_key_up_duration[MAX_KEY_NUM];

static int g_key_push_code[MAX_KEY_NUM];
static int g_key_push_duration[MAX_KEY_NUM];
static int g_key_push_period[MAX_KEY_NUM];

static int g_enable_key_push_no_need_keyup=0;

static int g_key_down_time[MAX_KEY_NUM];
#define MAX_COMBINE_KEY_NUM 16

static int g_combine_pin[MAX_KEY_NUM];
static int g_combine_keycode[MAX_KEY_NUM];
static int g_combine_count=0;
static unsigned int g_record_key_down=0;

//for beeper
static unsigned int g_beep_gpio_num;
static unsigned int g_beep_hz;
static unsigned int g_beep_duration;
static int g_beep_duration_count;
static int g_time_condition_byte=0;

#define TIMER_AUTO_REPEAT 1
#define TIMER_KEY_PUSH 2
static int g_timer_id=0;

static int g_beeper_is_set=0; //added by Jason-Yu

#define SEND_KEY(key) \
	if (g_beeper_is_set){ \
	   stop_beep_func(0); \
	   start_beep_pwm(); \
	}\
	handle_scancode(key, 1); \
	tasklet_schedule(&keyboard_tasklet);

void set_beep_pwm(int pin_number,int frequence,int duration);
void stop_beep_pwm(void);
void start_beep_pwm(void);
static void stop_beep_func(unsigned long none);

/* default keypad scancode table */
static unsigned int keypad_table[MAX_KEY_NUM] =
{
/*GPIO 0~16*/
/*     a     b     c     d enter space     -     =     1     2     3     4     5     6     7     8
    0x1e, 0x30, 0x2e, 0x20, 0x1c, 0x39, 0x0c, 0x0d, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 */
};

static unsigned int led_table[MAX_KEY_NUM] =
{
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

static unsigned int led_flash_slow=0, led_flash_fast=0; /* two flash check vectors */


static void auto_repeat_func(unsigned long scancode)
{
	if (g_timer_id==TIMER_KEY_PUSH){
		debug("send key push key\n");
		g_enable_key_push_no_need_keyup=1;
//printk("====210\n");
		SEND_KEY((unsigned char) scancode);
//printk("====212\n");
		if (g_key_push_period[g_auto_repeat_index]!=0){
			mod_timer(&auto_repeat_timer, jiffies + g_key_push_period[g_auto_repeat_index]);
		}
	}else if (g_timer_id==TIMER_AUTO_REPEAT){
		debug("send TIMER_AUTO_REPEAT key\n");
		SEND_KEY((unsigned char) scancode);
		mod_timer(&auto_repeat_timer, jiffies + auto_repeat_period[g_auto_repeat_index]);
		// jiffies is around 10ms on x86
	}
}


static void led_flash_slow_func(unsigned long speed)
{
	unsigned int output;
	if(led_flash_slow == 0)
	{
	   del_timer(&led_flash_slow_timer);
	   goto LED_FLASH_SLOW_EXIT;
	}
	output = readw(GPIO_DO);
	debug("timer1 output = %x \n", 0xFF & output);

	output = (output ^ led_flash_slow);
	debug("timer1 new output = %x \n", 0xFF & output);

	writew(output,GPIO_DO);
	mod_timer(&led_flash_slow_timer, jiffies + flash_speed[0]);

LED_FLASH_SLOW_EXIT:;
}


static void led_flash_fast_func(unsigned long speed)
{
	unsigned int output;
	if(led_flash_fast == 0)
	{
		del_timer(&led_flash_fast_timer);
		goto LED_FLASH_FAST_EXIT;
	}

	output = readw(GPIO_DO);
	debug("timer2 output = %x \n", 0xFF & output);

	output = (output ^ led_flash_fast);
	debug("timer2 new output = %x \n", 0xFF & output);

	writew(output,GPIO_DO);
	mod_timer(&led_flash_fast_timer, jiffies + flash_speed[1]);

LED_FLASH_FAST_EXIT:;
}


static void stop_beep_func(unsigned long none)
{
	debug("stop_beep_func\n");
	del_timer(&beep_timer);
	stop_beep_pwm();
}


/*
 * KEYPAD interrupt handler
 */
static inline int check_combine_key(int key_down){
	debug("check_combine_key,key_down=%x\n",key_down);
	int i;
	for(i=0;i<g_combine_count;i++){
		debug("g_combine_pin,g_combine_pin[i]=%x\n",g_combine_pin[i]);
		if (g_combine_pin[i]==key_down){
			//send virtual combine key
			SEND_KEY(g_combine_keycode[i]);
			return 1;
		}
	}
	return 0;
}


static void keypad61_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    int input, in, sel;
    int key_down_mask;
    int i;

    in = input = readw(GPIO_KEY_STATUS);
    debug("Input:0x%04x\n",in);

    key_down_mask = readw(GPIO_KEY_POL);

    if (input_active_low){
        key_down_mask = ~key_down_mask;
        debug ("320==Keypad Polarity is  %s\n", (input_active_low)? "Active_Low": "Active_High");
    }
    debug("KMask:0x%04x\n",key_down_mask);

    for (sel = 1, i=0; in != 0; i++, in >>= 1, sel <<= 1) {
        if ((in & 1) && (input_pins & sel)) {
            del_timer(&auto_repeat_timer);
            if (key_down_mask & sel) {
		        debug("\nGPIO%d up\n", i);
		        g_record_key_down&= ~( (unsigned int)(1<<i));
		        /* no need to send handle_scancode(x,0) */
		        /* replace by send handle_scancode(x,1) */
		        /* because we want to send a virtual key when key up */

		        /* handle_scancode(keypad_table[i], 0); */
                /* tasklet_schedule(&keyboard_tasklet); */
		        if (g_key_up_code[i]!=0 && g_enable_key_push_no_need_keyup==0){
			        //check up duration
			        if (g_key_up_duration[i]< jiffies-g_key_down_time[i]) {
				        SEND_KEY(g_key_up_code[i]);
			        }
		        }
	            g_enable_key_push_no_need_keyup=0;
                key_down_mask &= ~sel;
            }else{
		        debug("\nGPIO%d down\n", i);
		        key_down_mask |= sel;
		        g_key_down_time[i]=jiffies;

		        g_record_key_down|= 1<<i;
		        //check combine key
		        int has_combine=check_combine_key(g_record_key_down);
		        if (has_combine==1){
			        debug("\n combine key\n");
		            continue;
		        }

		        if (keypad_table[i]!=0){
			        debug("==send normal single key\n");
			        SEND_KEY(keypad_table[i]);
		        }

		        /* check key push */
		        if (g_key_push_code[i]!=0){
			        debug("\n key_push timer\n");
			        /* check up duration */
			        auto_repeat_timer.data = g_key_push_code[i];
			        g_timer_id=TIMER_KEY_PUSH;
//printk("====358\n");
		            g_auto_repeat_index=i;
//printk("====360\n");
			        mod_timer(&auto_repeat_timer, jiffies + g_key_push_duration[i]);
//printk("====362\n");
			        continue;
		        }else if (input & auto_pins){	//check auto repeat
		            debug("\n auto repeat timer\n");
		            auto_repeat_timer.data = keypad_table[i];
		            g_auto_repeat_index=i;
		            g_timer_id=TIMER_AUTO_REPEAT;
		            mod_timer(&auto_repeat_timer, jiffies + auto_repeat_duration[g_auto_repeat_index]);
                }
	        }
	    }
    } /* --for loop--- */

    if (input_active_low)
        key_down_mask = ~key_down_mask;

    writew(key_down_mask, GPIO_KEY_POL);
//printk("379---GPIO_KEY_POL = %x \n", readw(GPIO_KEY_POL));
    writew((input & input_pins), GPIO_KEY_STATUS);//Write same value to clear Interrupt
//printk("381---GPIO_KEY_STATUS = %x \n", readw(GPIO_KEY_STATUS));
}


static void led_handler(void)
{
	unsigned int i,sel,timer1_set=0, timer2_set=0;
	unsigned int output = 0;

	debug("     output : 0x%02x\n",output);
	debug("~input_pins : 0x%02x\n",0xFF & (~input_pins));
	debug("    PINMASK : 0x%02x\n",PIN_MASK);

	for(sel=1,i = 0; i<MAX_KEY_NUM; i++, sel<<=1) {
	   debug("sel : 0x%x\n",sel);
	   if(sel & (~input_pins) & PIN_MASK)
	     {
		switch(led_table[i]){
		case 0:
			led_flash_slow &= (~sel);
			led_flash_fast &= (~sel);
			break;
		case 1:
			led_flash_slow &= (~sel);
			led_flash_fast &= (~sel);
			output = output | sel;
			break;
		case 2:
			led_flash_fast &= (~sel);
			output = output | sel;
			led_flash_slow |= sel;
			timer1_set = 1;
			break;
		case 3:
			led_flash_slow &= (~sel);
			output |= sel;
			led_flash_fast |= sel;
			timer2_set = 1;
			break;
		default:
			led_flash_slow &= (~sel);
			led_flash_fast &= (~sel);
			debug("LED%d set err!\n",i);
			}
		}
	}

	if(timer1_set)
		mod_timer(&led_flash_slow_timer, jiffies + flash_speed[0]);
	if(timer2_set)
		mod_timer(&led_flash_fast_timer, jiffies + flash_speed[1]);

	debug("set output : 0x%02x\n",output);
	debug("led_flash_slow: 0x%02x\n",led_flash_slow);
	debug("led_flash_fast: 0x%02x\n",led_flash_fast);

	writew(output,GPIO_DO);
}


static struct ctl_table_header *keypad_sysctl_header;

/*SET UP LED min and max value individually*/
static int led_min[MAX_KEY_NUM] =
{
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};


static int led_max[16] =
  {
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
  };


static int led_proc_handler(ctl_table *ctl, int write, struct file *filp, void *buffer, size_t *lenp)
{
    int ret;
    /*add min and max value check*/
    ret = proc_dointvec_minmax(ctl, write, filp, buffer, lenp);
    if (write && (ctl->ctl_name == DEV_KEYLED)){
		led_handler(); //show on the led
    }

    return ret;
}

ctl_table keypad_dev_table[] =
{
    {DEV_KEYPAD, "keypad", keypad_table, 8*sizeof(int), 0644, NULL, &proc_dointvec},
    {DEV_KEYLED,    "led",    led_table, 8*sizeof(int), 0644, NULL, &led_proc_handler,&sysctl_intvec, NULL, led_min, led_max},
    {0}
};

ctl_table keypad_root_table[] =
{
#ifdef CONFIG_PROC_FS
    {CTL_DEV, "dev", NULL, 0, 0555, keypad_dev_table},
#endif /* CONFIG_PROC_FS */
    {0}
};


static int gpio_read_proc(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	int i,sel, clen=0;

	for(sel=1,i = 0; i<MAX_KEY_NUM; i++, sel<<=1) {
		if(((input_pins & sel) & PIN_MASK) != 0) {
	    	clen+=sprintf(buf+clen,"GPIO_%d: key = '0x%02x'\n",i,keypad_table[i]);
		} else {
			if((sel & PIN_MASK) != 0) {
                //clen+=sprintf(buf+clen,"GPIO_%d: LED = '0x%02x'\n",i,led_table[i]);
                clen+=sprintf(buf+clen,"GPIO_%d: LED = ",i);
				switch(led_table[i]){
					case 0:
						clen+=sprintf(buf+clen,"Off\n");
						break;
					case 1:
						clen+=sprintf(buf+clen,"On\n");
						break;
					case 2:
						clen+=sprintf(buf+clen,"Flash slow\n");
						break;
					case 3:
						clen+=sprintf(buf+clen,"Flash fast\n");
						break;
					default:
						clen+=sprintf(buf+clen,"ERR \n");
						break;
				}
			} else {
				clen+=sprintf(buf+clen,"GPIO_%d: Disabled. \n",i);
            }
		}
	}

	if (clen > offset)
		clen -= offset;
	else
		clen = 0;

	return clen < len ? clen : len;
}


int __init keypad61_init (void)
{

#ifdef CONFIG_ARCH_PL1029
    /* for pl1029*/
    if (pl_get_dev_hz() == 96000000) {          /* dclk = 96MHz */
        writeb(5, PL_CLK_GPIO);                 /* program gpio dev clk to 48MHz */
    } else if (pl_get_dev_hz() == 32000000) {   /* dclk = 32MHz */
        writeb(4, PL_CLK_GPIO);                 /* program gpio dev clk to 32MHz */
    } else if (pl_get_dev_hz() == 120000000) {  /* dclk = 120MHz */
        writeb(6, PL_CLK_GPIO);                 /* program gpio dev clk to 30MHz */
    } else  {
        printk("Keypad driver cannot support device clock as %d\n", pl_get_dev_hz());
        return -EINVAL;
    }

#endif

    int rc = 0;
    /* Software reset PWM */
    writeb( readb(GPIO_RSTN_63) | RSTN_PWM_63, GPIO_RSTN_63);
    //printk("528--GPIO_KEY_PERIOD=%x\n", readl(GPIO_KEY_PERIOD));
    // writel( readl(GPIO_KEY_PERIOD) | (input_pins<<8), GPIO_KEY_PERIOD);   /* for PL1063*/
    writeb( readl(GPIO_KEY_PERIOD), GPIO_KEY_PERIOD);   /* for PL1029 trigger by level*/

    /* Config PWM Timer */
    writel(0x80808080, GPIO_TIMER_PRESCALER);
    writel(0, GPIO_TIMER_CFG);
    writel(0, GPIO_TIMER_DZ4_7);
    writel(0, GPIO_TIMER_DZ0_3);

    /* Set power driveing */
    writew(0, GPIO_DRIVE_E2);
    writew(0, GPIO_DRIVE_E4);
    writew(0, GPIO_DRIVE_E8);
    writel(0, GPIO_TIMER_TCON);

    /* config input GPIO */

    debug("GPIO_OE=%x\n",(~input_pins & PIN_MASK) );

    writew( readw(GPIO_ENB) | input_pins , GPIO_ENB );

//printk("GPIO_ENB = %x \n", readw(GPIO_ENB));


    if (input_active_low){
        writew(readw(GPIO_PF0_PU) | input_pins, GPIO_PF0_PU);                /* enable pull up for active low input pin */
        debug ("579==Keypad Polarity is %s\n", (input_active_low)? "Active_Low": "Active_High");
    }else{
        writew(readw(GPIO_PF0_PD) | input_pins, GPIO_PF0_PD);                /* enable pull down for input pin */
        debug ("582==Keypad Polarity is %s\n", (input_active_low)? "Active_Low": "Active_High");
    }
    //printk("583====GPIO_PF0_PD is %x====\n", readw(GPIO_PF0_PD));
    writew(readw(GPIO_KEY_MASK) | input_pins, GPIO_KEY_MASK);            /* enable input pin interrupt mask */
    writew(readw(GPIO_KEY_ENB) | input_pins, GPIO_KEY_ENB);               /* keypad release and press controlling */

//printk("GPIO_KEY_MASK = %x \n", readw(GPIO_KEY_MASK));
//printk("GPIO_KEY_ENB = %x \n", readw(GPIO_KEY_ENB));


    writew(readw(GPIO_PF1_SMT) | input_pins, GPIO_PF1_SMT);           /* enable schmitt trigger for input pin */


    writeb( readb(GPIO_KEY_PERIOD) | KEYPAD_SENSIBILITY, GPIO_KEY_PERIOD); /* for PL1063*/


    if (input_active_low){
        writew(readw(GPIO_KEY_POL) | input_pins, GPIO_KEY_POL);
        debug ("600==Keypad Polarity is %s\n", (input_active_low)? "Active_Low": "Active_High");
    }else{
       writew(readw(GPIO_KEY_POL) | (~input_pins & 0xFF) , GPIO_KEY_POL);          /* key pressing */
       debug ("603==Keypad Polarity is %s\n", (input_active_low)? "Active_Low": "Active_High");
    }
//printk("GPIO_KEY_POL = %x \n", readw(GPIO_KEY_POL));
    /* Software reset GPIO KEY */
    writeb( readb(GPIO_RSTN_63) | RSTN_PWM_63 | RSTN_KEY_63 | RSTN_INT_63 , GPIO_RSTN_63); /*for pl1063*/
//printk("GPIO_RSTN_63 = %x \n", readw(GPIO_RSTN_63));


    /* clear all KEY status and request irq */
    writew(readw(GPIO_KEY_STATUS), GPIO_KEY_STATUS);    /* enable all GPIO pin */
//printk("GPIO_KEY_STATUS = %x \n", readw(GPIO_KEY_STATUS));

    /* Share IRQ for the other GPIO input controlling */
    //if (request_irq(IRQ_PL_PWM, keypad61_handler, SA_SHIRQ | SA_SAMPLE_RANDOM, "keypad", NULL))
//printk("@@@ request_irq @@@\n");
int k = request_irq(IRQ_PL_PWM, keypad61_handler,  SA_SHIRQ |SA_SAMPLE_RANDOM, "keypad", "dev_keypad") ;
//printk(" ====k is %d \n");
      if(k)
	{
        printk(KERN_WARNING"KEYPAD, LED allocate IRQ failed!\n");
//printk("!!!!! Crash~~~~\n");
        rc = -ENODEV;
        goto EXIT;
    }

    init_timer(&auto_repeat_timer);			/*keypad repeat timer*/
    auto_repeat_timer.function = auto_repeat_func;
    auto_repeat_timer.data = 0;

    init_timer(&led_flash_slow_timer);		/*LED flash timer1*/
    led_flash_slow_timer.function = led_flash_slow_func;
    led_flash_slow_timer.data = 0;

    init_timer(&led_flash_fast_timer);		/*LED flash timer2*/
    led_flash_fast_timer.function = led_flash_fast_func;
    led_flash_fast_timer.data = 0;

    init_timer(&beep_timer);		/*beep timer*/
    beep_timer.function = stop_beep_func;
    beep_timer.data = 0;

    MOD_INC_USE_COUNT;


    /* create proc entry */
	create_proc_read_entry("gpio", 0, NULL, gpio_read_proc, NULL);
    /* create sysctl entry */
	keypad_sysctl_header = register_sysctl_table(keypad_root_table, 1);

    /* set beeper */
    /* g_beep_gpio_num default value is 0, to avoid  GPIO 0 conflick with other GPIO*/
    if ( g_beeper_is_set )
   	   set_beep_pwm(g_beep_gpio_num,g_beep_hz,g_beep_duration);

    //writew( temp, GPIO_KEY_POL);
    // enable_irq(IRQ_PL_PWM);
    printk("Prolific keypad driver v%s\n", VERSION);

EXIT:
    if (rc < 0)
	{
        free_irq(IRQ_PL_PWM, NULL);
    }
/*
printk("==== 0xb94c0054 (GPIO_ENB) is %08x\n", readl(GPIO_ENB));
printk("==== 0xb94c0058 (GPIO_OE) is %08x\n", readl(GPIO_OE));
printk("==== 0xb94c0064 (GPIO_PF0_PU) is %08x\n", readl(GPIO_PF0_PU));
printk("==== 0xb94c0078 (GPIO_KEY_ENB) is %08x\n", readl(GPIO_KEY_ENB));
printk("==== 0xb94c007c (GPIO_KEY_PERIOD) is %08x\n", readl(GPIO_KEY_PERIOD));
printk("==== 0xb94c0080 (GPIO_KEY_MASK) is %08x\n", readl(GPIO_KEY_MASK));
*/
    return rc;
}


void __exit keypad61_exit (void)
{
    //CWC
    remove_proc_entry("gpio", NULL);	//release entry
    unregister_sysctl_table(keypad_sysctl_header);//release table
    //CWC END
    free_irq(IRQ_PL_PWM, NULL);
    MOD_DEC_USE_COUNT;
}

/*
 * Grammar : pl1061.keypad={low;}<pin>{;<map>{;<rpt>}}
 *       <pin> := pin:<pin mask>{,<input mask>{,<auto repeat pin>}}
 *       <map> := map:<input 1 scancode>,<input 2 scancode>...
 *       <rpt> := rpt:<delay>{,<period>}
 *       low   := low active
 *       <pin mask> := hexdecimal, the pin will config to GPIO in/out, validate value is 0 ~ 3ff
 *       <input mask> := hexdecimal, config input pin
 *       <input N scancode> := hexdecimal value, assign scancode to <input mask> by LSB order
 *       <auto repeat pin> := hexdecimal value, config auto repreat pin
 *       <delay> := hexdecimal value, unit is 1/100 second, the first repeat in <delay> time
 *       <period> := hexdecimal value, unit is 1/100 second, after the first repeat the key auto-generate in <period>
 * Example : pl1061.keypad=pin:ff,f0;map:1e,30,2e,20,1c,39,0c,0d;rpt:
 */

//i==0,set duration
//i==1,set period
void set_auto_repeat_all(int i,int value){
	int j;
	for(j=0;j<MAX_KEY_NUM;j++){
		if (i==0) auto_repeat_duration[j]=value;
		if (i==1) auto_repeat_period[j]=value;
	}
}

char *check_arg_pin(char *options){
    int in, hex, i;
    in = input_pins;
    if (strncmp(options, "low;", 4) == 0) {
        options += 4;
        input_active_low = 1;
    }

    if (strncmp(options, "pin:", 4) == 0) {
        options += 4;
        in = input_pins = PIN_MASK = simple_strtoul(options, &options, 16);
        if (*options == ',') {
            options++;
            in = input_pins = PIN_MASK & simple_strtoul(options, &options, 16);
            if (*options == ',') {
                options++;
                auto_pins = input_pins & simple_strtoul(options, &options, 16);
            }
        }
    }

    if (*options == ';') {
        options++;
        //debug("in=%x\n",in);
        if (strncmp(options, "map:", 4) == 0) {
            options +=4;
            for(i = 0; i < MAX_KEY_NUM && in != 0 && options != NULL && *options != '\0' && *options != ';' ; i++, in >>= 1) {
                if ((in & 1) == 0)
                    continue;
                hex = simple_strtoul(options, &options, 16);
                keypad_table[i] = hex;
                debug("keypad_table[%d]=%x\n",i,hex);
                if (options && *options == ',')
                    options++;
            }
        }

        if (*options == ';') {
            options++;
            if (strncmp(options, "rpt:", 4) == 0) {
                options+=4;
                for(i = 0; i < 2 && options != NULL && *options != '\0' && *options != ';' ; i++) {
                    hex = simple_strtoul(options, &options, 16);
	// debug("auto_repeat[%d]=%x\n",i, HZ*hex/100);
	//auto_repeat[i] = HZ*hex/100;
 	 set_auto_repeat_all(i,HZ*hex/100);
                    if (options && *options == ',')
                        options++;
	//debug("after rtp,option=%s\n",options);
                }
            }
        }
    }
	return options;
}


void set_gpio_out(int pin_number){
	PIN_MASK |= 1 << pin_number;
	input_pins &= ~(1 << pin_number);
	debug("[set_gpio_out]number=%d,PIN_MASK=%x,input_pins=%x\n",pin_number,PIN_MASK,input_pins);
}

void set_gpio_in(int pin_number){
	PIN_MASK |= 1 << pin_number;
	input_pins |= 1 << pin_number;
	//printk("796--[set_gpio_in]number=%d,PIN_MASK=%x,input_pins=%x\n",pin_number,PIN_MASK,input_pins);
	debug("[set_gpio_in]number=%d,PIN_MASK=%x,input_pins=%x\n",pin_number,PIN_MASK,input_pins);
}

void set_key_down(int pin_number,int code,int duration,int period){
	keypad_table[pin_number]=ascii_to_scancode(code);
	if (duration!=0 || period!=0){
		auto_pins|= 1 << pin_number;
		auto_repeat_duration[pin_number]= HZ*duration/1000;
		auto_repeat_period[pin_number]=HZ*period/1000;
		debug("auto_pins=%x,duration=%x,period=%x\n",auto_pins, auto_repeat_duration[pin_number], auto_repeat_period[pin_number]);
	}
}

void set_key_up(int pin_number,int code,int condition){
	g_key_up_code[pin_number]=ascii_to_scancode(code);
	g_key_up_duration[pin_number]=HZ*condition/1000;
}

void set_key_push(int pin_number,int code,int condition,int auto_repeat_period){
	g_key_push_code[pin_number]=ascii_to_scancode(code);
	g_key_push_duration[pin_number]=HZ*condition/1000;
	g_key_push_period[pin_number]=HZ*auto_repeat_period/1000;
	debug("g_key_push_duration[pin_number]=%d\n",g_key_push_duration[pin_number]);
	debug("g_key_push_period[pin_number]=%d\n",g_key_push_period[pin_number]);
}

void set_key_combine(int main_pin_number,int pin_number,int code){
	if (g_combine_count<MAX_COMBINE_KEY_NUM){
		g_combine_pin[g_combine_count] |= 1 << main_pin_number;
		g_combine_pin[g_combine_count] |= 1 << pin_number;
		g_combine_keycode[g_combine_count] =ascii_to_scancode(code);
		g_combine_count++;
	}
}

/*
 * New Grammer:
 * for example:
 * pl1061.keypad=low;GPIO_IN(0,1,3,4,5,6,7);\
 * KEY_DOWN([0,i,1000ms,2000ms],[1,j,2000ms,4000ms],[3,l],[4,5],[5,6],[6,=],[7,-]);\
 * KEY_UP([1,t,>30ms]);KEY_COMBINE(1,[3,r],[4,e]);\
 * KEY_PUSH([1,f,>3000ms],[3,g,>3000ms]);BEEP(2,501hz,200ms) \
 */

#define SKIP_CHARS(x,str) \
    while(strchr(str, *x)) x++;

char * check_arg_gpio_pola(char *options)
{
    if (strncmp(options, "low", 3)==0){
        input_active_low = 1;
	    options+=4;
    }
    return options;
}


char * check_arg_gpio_oe(char *options){
	int i;
	int value;

    SKIP_CHARS(options, "; ");
	if (*options == '\0' ) return options;

	if (strncmp(options, "GPIO_OE(", 8) == 0) {
		options+=8;
		for(i = 0; i < MAX_KEY_NUM && options != NULL && *options != '\0' && *options != ')' ; i++) {
			value = simple_strtoul(options, &options, 10);
			debug("[GPIO_OE]value=%d\n",value);
			set_gpio_out(value);
			if (options && *options == ',')
				options++;
		}
		if (options && *options == ')')
			options++;
		if (options && *options == ';')
			options++;
	}
	return options;
}

char * check_arg_gpio_in(char *options){
	int i;
	int value;

    SKIP_CHARS(options, "; ");
	if (*options == '\0' ) return options;

	if (strncmp(options, "GPIO_IN(", 8) == 0) {
		options+=8;
		for(i = 0; i < MAX_KEY_NUM && options != NULL && *options != '\0' && *options != ')' ; i++) {
			value = simple_strtoul(options, &options, 10);
			debug("[GPIO_IN]value=%d\n",value);
			set_gpio_in(value);
			if (options && *options == ',')
				options++;
		}
		if (options && *options == ')')
			options++;
		if (options && *options == ';')
			options++;
	}
	return options;
}


#define SKIP_CHAR(x,cc) \
	while(*x==cc) x++


char * check_arg_key_down(char *options){
	int i;
	int gpio_num=0;
	int key_code=0;
	int duration=0;
	int period=0;
//	debug("[check_arg_key_down] options=%s\n",options);
	if (options==NULL) return "";
    SKIP_CHARS(options, "; ");

//	SKIP_CHAR(options,';');
	if (strncmp(options, "KEY_DOWN(", 9) == 0) { //-----1-----
		options+=9;
		for(i = 0; i < MAX_KEY_NUM && options != NULL && *options != '\0' && *options != ')' ; i++) {
			SKIP_CHAR(options,' ');
			if (options && *options == '['){
				options++;
				SKIP_CHAR(options,' ');
				//get gpio_num,<key_code><duration> <period>
				gpio_num = simple_strtoul(options, &options, 10);
				SKIP_CHAR(options,',');
				SKIP_CHAR(options,' ');
				key_code=*options;  // keycode is ASCII code
				options++;
				SKIP_CHAR(options,' ');
				if (*options!=']'){
					SKIP_CHAR(options,',');
					duration = simple_strtoul(options, &options, 10);
					SKIP_CHAR(options,'m');
					SKIP_CHAR(options,'s');
					SKIP_CHAR(options,' ');
					SKIP_CHAR(options,',');
					SKIP_CHAR(options,' ');
					period = simple_strtoul(options, &options, 10);
					SKIP_CHAR(options,'m');
					SKIP_CHAR(options,'s');
					debug("[key_down],gpio=%d,code=%x,duraton=%dms,period=%d ms\n",	gpio_num,key_code,duration,period);
					set_key_down(gpio_num,key_code,duration,period);
				}else{
					debug("[key_down],gpio=%d,code=%x\n",gpio_num,key_code);
					set_key_down(gpio_num,key_code,0,0);
					}
				SKIP_CHAR(options,' ');
				SKIP_CHAR(options,']');
			}else{
				break;
 				}
			SKIP_CHAR(options,',');
			}//---for loop---
			SKIP_CHAR(options,')');
			SKIP_CHAR(options,';');
		}//-----1-----
	return options;
}


char * check_arg_key_up(char *options){
	int i;
	int gpio_num=0;
	int key_code=0;
	int condition=0;
//	debug("[check_arg_key_up] options=%s\n",options);
	if (options==NULL) return "";
//	SKIP_CHAR(options,';');
    SKIP_CHARS(options, "; ");
	if (strncmp(options, "KEY_UP(", 7) == 0) {
		options+=7;
		for(i = 0; i < MAX_KEY_NUM && options != NULL && *options != '\0' && *options != ')' ; i++) {
			SKIP_CHAR(options,' ');
			if (options && *options == '['){
				options++;
				SKIP_CHAR(options,' ');
				//get gpio_num,<key_code><duration> <period>
				gpio_num = simple_strtoul(options, &options, 10);
				SKIP_CHAR(options,',');
				//key_code = simple_strtoul(options, &options, 16);
				SKIP_CHAR(options,' ');
				key_code=*options;
				options++;
				SKIP_CHAR(options,' ');
				if (*options!=']'){
					SKIP_CHAR(options,',');
					SKIP_CHAR(options,'>');
					condition = simple_strtoul(options, &options, 10);
					SKIP_CHAR(options,'m');
					SKIP_CHAR(options,'s');
					SKIP_CHAR(options,' ');
					SKIP_CHAR(options,',');
					SKIP_CHAR(options,' ');
					debug("[key_up],gpio=%d,code=%x,condition=%dms\n",	gpio_num,key_code,condition);
					set_key_up(gpio_num,key_code,condition);
				}else{
					debug("[key_down],gpio=%d,code=%x\n",gpio_num,key_code);
					set_key_up(gpio_num,key_code,20);
				}
				SKIP_CHAR(options,' ');
				SKIP_CHAR(options,']');
			}else{
				break;
			}
			SKIP_CHAR(options,',');
		}
		SKIP_CHAR(options,')');
		SKIP_CHAR(options,';');
	}
	return options;
}


char * check_arg_key_combine(char *options){
	int i;
	int main_gpio=0;
	int gpio_num=0;
	int key_code=0;
//	debug("[check_arg_key_combine] options=%s\n",options);
	if (options==NULL) return "";
//	SKIP_CHAR(options,';');
    SKIP_CHARS(options, "; ");

	if (strncmp(options, "KEY_COMBINE(", 12) == 0) {
		options+=12;
		main_gpio = simple_strtoul(options, &options, 10);
		SKIP_CHAR(options,',');
		for(i = 0; i < MAX_KEY_NUM && options != NULL && *options != '\0' && *options != ')' ; i++) {
			SKIP_CHAR(options,' ');
			if (options && *options == '['){
				options++;
				SKIP_CHAR(options,' ');
				//get gpio_num,<key_code><duration> <period>
				gpio_num = simple_strtoul(options, &options, 10);
				SKIP_CHAR(options,',');
				//key_code = simple_strtoul(options, &options, 16);
				SKIP_CHAR(options,' ');
				key_code=*options;
				options++;
				SKIP_CHAR(options,' ');
				debug("[key_COMBINE],gpio=(%d,%d),code=%x\n",main_gpio,gpio_num,key_code);
				set_key_combine(main_gpio,gpio_num,key_code);
				SKIP_CHAR(options,' ');
				SKIP_CHAR(options,']');
			}else{
				break;
			}
			SKIP_CHAR(options,',');
		}
		SKIP_CHAR(options,')');
		SKIP_CHAR(options,';');
	}
	return options;
}


char * check_arg_key_push(char *options){
	int i;
	int gpio_num=0;
	int key_code=0;
	int condition=0;
	int period=0;
	debug("[check_arg_key_up] options=%s\n",options);
	if (options==NULL) return "";
	// SKIP_CHAR(options,';');
    SKIP_CHARS(options, "; ");

	if (strncmp(options, "KEY_PUSH(", 9) == 0) {
		options+=9;
		for(i = 0; i < MAX_KEY_NUM && options != NULL && *options != '\0' && *options != ')' ; i++) {
			SKIP_CHAR(options,' ');
			if (options && *options == '['){
				options++;
				SKIP_CHAR(options,' ');
				//get gpio_num,<key_code><duration> <period>
				gpio_num = simple_strtoul(options, &options, 10);
				SKIP_CHAR(options,',');
				//key_code = simple_strtoul(options, &options, 16);
				SKIP_CHAR(options,' ');
				key_code=*options;
				options++;
				SKIP_CHAR(options,' ');
				if (*options!=']'){
					SKIP_CHAR(options,',');
					SKIP_CHAR(options,'>');
					condition = simple_strtoul(options, &options, 10);
					SKIP_CHAR(options,'m');
					SKIP_CHAR(options,'s');
					SKIP_CHAR(options,' ');
					SKIP_CHAR(options,',');
					SKIP_CHAR(options,' ');

					period=simple_strtoul(options, &options, 10);
					SKIP_CHAR(options,'m');
					SKIP_CHAR(options,'s');
					debug("[key_push],gpio=%d,code=%x,condition=%dms,period=%d ms\n",gpio_num,key_code,condition,period);
					set_key_push(gpio_num,key_code,condition,period);
				}else{
					debug("[key_push],gpio=%d,code=%x\n",	gpio_num,key_code);
					set_key_push(gpio_num,key_code,2000,0);
				}
				SKIP_CHAR(options,' ');
				SKIP_CHAR(options,']');
			}else{
				break;
			}
			SKIP_CHAR(options,',');
		}
		SKIP_CHAR(options,')');
		SKIP_CHAR(options,';');
	}
	return options;
}

//pin_number: 0-7
//duration: xxx ms
//frequence : Hz (for example: 1000, 1khz)
void set_beep_pwm(int pin_number,int frequence,int duration){
	debug("pin_number=%d\n",pin_number);
	debug("duration=%d\n",duration);
	debug("frequence=%d\n",frequence);

	g_beep_duration_count=HZ*duration/1000;
	//the duration currently not work,because
	//auto reload bit hardware not work.

	int pin_byte= (1L<< pin_number);

	writel(0, GPIO_TIMER_TCON);
         debug("pin_byte is 0x%x\n", ~(pin_byte));
	writew(readw(GPIO_ENB) & ~(pin_byte), GPIO_ENB);


	writeb( readw(GPIO_OE) |  pin_byte , GPIO_OE);
	debug("set_beep_pwm_GPIO_ENB=%x\n",readw(GPIO_ENB));
	//configure timer prescale //
	writel(0xffffffff, GPIO_TIMER_PRESCALER);//prescaler value=255,

	//output clock period=128khz
	writel(0, GPIO_TIMER_CFG);

	//compute TLE value
	unsigned char c=(128000/4)/frequence; // mod 4 because 1/2clock,the count value is both for high and low

	debug("c=%x\n",c);
	debug("frequence=%x\n",frequence);

	writew(c, GPIO_TIMER_TLE0B+pin_number*(8)); //because 8 bytes for TLE0B to TLE3B
	writew(c, GPIO_TIMER_TLE1B+pin_number*(8));
	writew(0x0, GPIO_TIMER_TLE2B+pin_number*(8));

	unsigned char repeat_no=g_beep_duration_count & 0xff;
	if (g_beep_duration_count> 0xff){
		repeat_no=0xff;
	}

	debug("pin_number is %d, g_beep_duration_count=%x, repeat_no=%x\n",pin_number, g_beep_duration_count, repeat_no);
	writew(repeat_no, GPIO_TIMER_TLE3B+pin_number*(8));//repeat no
        // enable timer 1//
	unsigned int time_bytes;

#ifdef USING_STOP_BEEP_TIMER
	time_bytes=0x03; //auto reload
#else
	time_bytes=0x05; //one shot,but repeat no not work
#endif

	g_time_condition_byte=time_bytes<<(4*pin_number);
	writew(readw(GPIO_KEY_MASK) & ~pin_byte, GPIO_KEY_MASK); // enable input pin interrupt mask
}


void start_beep_pwm(){
	debug("g_time_condition_byte=%x\n",g_time_condition_byte);
	debug("time con=%x\n",readl(GPIO_TIMER_TCON));
	writel(g_time_condition_byte, GPIO_TIMER_TCON);

	//set timer to stop beep
#ifdef USING_STOP_BEEP_TIMER
	mod_timer(&beep_timer, jiffies + g_beep_duration_count);
	debug("set stop beep timer,g_beep_duration_count=%d\n",g_beep_duration_count);

#endif
}


void stop_beep_pwm(){
    debug("[stop_beep_pwm]\n");
    writel(0, GPIO_TIMER_TCON);
}


int check_arg_beep(char *options){
	int gpio_num=0;
	int hz=0;
	int duration=0;
	debug("[check_arg_key_up] options=%s\n",options);
	if (options==NULL) return 0;
	// SKIP_CHAR(options,';');
    SKIP_CHARS(options, "; ");

	if (strncmp(options, "BEEP(", 5) == 0) {
		options+=5;
		//get gpio_num,<key_code><hz> <duration>
		gpio_num = simple_strtoul(options, &options, 10);
		SKIP_CHAR(options,',');

		hz = simple_strtoul(options, &options, 10);
		SKIP_CHAR(options,'h');
		SKIP_CHAR(options,'z');
		SKIP_CHAR(options,' ');
		SKIP_CHAR(options,',');

		duration = simple_strtoul(options, &options, 10);
		SKIP_CHAR(options,'m');
		SKIP_CHAR(options,'s');
		SKIP_CHAR(options,' ');
		SKIP_CHAR(options,')');
		SKIP_CHAR(options,';');

		g_beep_gpio_num=gpio_num;
		g_beep_hz=hz;
		g_beep_duration=duration;
		g_beeper_is_set=1;
	}
	return g_beeper_is_set;
}


int __init keypad61_setup(char *options)
{
    // printk("Keypad GPIO_BASE is %x \n", rGPIO_BASE);
    debug("====input options=%s\n",options);

    if (strncmp(options, "pin:", 4) == 0){  /* OLD Grammar:*/
        debug("====keypad61_setup option=%s\n",options);
	    options=check_arg_pin(options);
    } else {                                /*NEW Grammar*/
        debug("====1 options=%s\n",options);
        options=check_arg_gpio_pola(options);
        debug("====1 options=%s\n",options);
	    options=check_arg_gpio_oe(options);
        debug("====2 options=%s\n",options);
	    options=check_arg_gpio_in(options);
        debug("====3 options=%s\n",options);
        options=check_arg_key_down(options);
        debug("====4 options=%s\n",options);
	    options=check_arg_key_up(options);
        debug("====5 options=%s\n",options);
	    options=check_arg_key_combine(options);
        debug("====6 options=%s\n",options);
	    options=check_arg_key_push(options);
        debug("====7 options=%s\n",options);
	    check_arg_beep(options);
    }
    return 1;
}


__setup("prolific.keypad=", keypad61_setup);

module_init(keypad61_init);
module_exit(keypad61_exit);

MODULE_AUTHOR("Jedy Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("Prolific pl1061 GPIO keypad driver");
