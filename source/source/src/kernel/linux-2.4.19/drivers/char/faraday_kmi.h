#ifndef FARADAY_KEYB_H
#define FARADAY_KEYB_H

// --------------------------------------------------------------------
//	2002-12-26: lmc83
//		- based on <linux/pc_keyb.h>
// --------------------------------------------------------------------

/*
 *	Internal variables of the driver
 */

extern unsigned char pckbd_read_mask;


#define MOUSE_ENABLE 0xF4

#define KEYPAD_INT        	0x04
#define KEYBOARD_TXINT    	0x02
#define KEYBOARD_RXINT    	0x01


/* register */

#define KMI_Control            	0x0
#define KMI_SampleRate        	0x4
#define KMI_RequestToSend     	0x8
#define KMI_Status             	0xC
#define KMI_IntStatus        	0x10
#define KMI_Receive         	0x14
#define KMI_Transmit         	0x18
#define KMI_Keypad_X        	0x30
#define KMI_Keypad_Y         	0x34
#define KMI_AutoscanPeriod     	0x38

/* control register */

#define Autoscan_two_key		0x800
#define Clear_PADINT			0x400
#define Enable_Autoscan			0x200
#define EnableKeypad			0x100
#define Clear_RX_INT			0x80
#define Clear_TX_INT			0x40
#define Line_Control			0x0
#define No_Line_Control			0x20
#define Enable_RX_INT			0x10
#define Enable_TX_INT			0x8
#define Enable_KMI				0x4
#define Force_data_line_low		0x2
#define Force_clock_line_low	0x1

/* Status register */

#define RX_Busy					0x2
#define TX_Busy					0x8
#define RX_Full					0x4
#define TX_Empty				0x10



#define KBD_TIMEOUT 1000                /* Timeout in ms for keyboard command acknowledge */


// --------------------------------------------------------------------
//		keyboard 相關資料結構
// --------------------------------------------------------------------
/*
 *	Keyboard Commands
 */

#define KBD_CMD_SET_LEDS	0xED	/* Set keyboard leds */
#define KBD_CMD_SET_RATE	0xF3	/* Set typematic rate */
#define KBD_CMD_ENABLE		0xF4	/* Enable scanning */
#define KBD_CMD_DISABLE		0xF5	/* Disable scanning */
#define KBD_CMD_RESET		0xFF	/* Reset */
#define KBD_CMD_CODE_SET	0xF0	/* 設定 scan code set */

/*
 *	Keyboard Replies
 */

#define KBD_REPLY_POR		0xAA	/* Power on reset */
#define KBD_REPLY_ACK		0xFA	/* Command ACK */
#define KBD_REPLY_RESEND	0xFE	/* Command NACK, send the cmd again */





// --------------------------------------------------------------------
//		mouse 相關資料結構
// --------------------------------------------------------------------
/*
 *  Mouse Commands
 */
#define AUX_SET_RES			0xE8	/* Set resolution */
#define AUX_SET_SCALE11		0xE6	/* Set 1:1 scaling */
#define AUX_SET_SCALE21		0xE7	/* Set 2:1 scaling */
#define AUX_GET_SCALE		0xE9	/* Get scaling factor */
#define AUX_SET_STREAM		0xEA	/* Set stream mode */
#define AUX_SET_SAMPLE		0xF3	/* Set sample rate */
#define AUX_ENABLE_DEV		0xF4	/* Enable aux device */
#define AUX_DISABLE_DEV		0xF5	/* Disable aux device */
#define AUX_RESET			0xFF	/* Reset aux device */
#define AUX_ACK				0xFA	/* Command byte ACK. */

#define AUX_BUF_SIZE		2048	/* This might be better divisible by
					   				   three to make overruns stay in sync
					   				   but then the read function would need
					   				   a lock etc - ick */
struct aux_queue {
	unsigned long head;
	unsigned long tail;
	wait_queue_head_t proc_list;
	struct fasync_struct *fasync;
	unsigned char buf[AUX_BUF_SIZE];
};





#endif