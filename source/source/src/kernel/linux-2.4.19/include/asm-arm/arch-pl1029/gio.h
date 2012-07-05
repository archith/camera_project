#ifndef __ASM_ARCH_GIOS_H
#define __ASM_ARCH_GIOS_H

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

#define GPIO_ENB                	(rGPIO_BASE + 0x54)
#define GPIO_DO                 	(rGPIO_BASE + 0x56)  /* Data Ouput */
#define GPIO_OE                 	(rGPIO_BASE + 0x58)  /* 1:Output, 0:Input Selection */
#define GPIO_DRIVE_E2           (rGPIO_BASE + 0x5a)  /* E2, E4, E8 Power Driving */
#define GPIO_DI                 	(rGPIO_BASE + 0x5c)  /* Data input */
#define GPIO_DRIVE_E4           (rGPIO_BASE + 0x60)
#define GPIO_DRIVE_E8           (rGPIO_BASE + 0x62)
#define GPIO_PF0_PU             (rGPIO_BASE + 0x64)
#define GPIO_PF0_PD             (rGPIO_BASE + 0x66)
#define GPIO_PF1_SMT            (rGPIO_BASE + 0x68)
#define GPIO_INT_STATUS         (rGPIO_BASE + 0x70)

#define GPIO_KEY_ENB            	(rGPIO_BASE + 0x78)  /* Keypad enable */
#define GPIO_KEY_POL            	(rGPIO_BASE + 0x7a)  /* Keypad input polarity select 0: rasing edge 1: falling edge */
#define GPIO_KEY_PERIOD         	(rGPIO_BASE + 0x7c)  /* Keypad Number from 0 to 255 for Debounce */
#define GPIO_KEY_LEVEL          	(rGPIO_BASE + 0x7d)  /* Key Level or edge trigger select 0: level, 1: edge*/
#define GPIO_RSTN_61            (rGPIO_BASE + 0x7e)  /*for PL1061*/
#define RSTN_CIR_61             	(1L << (29-16))
#define RSTN_KEY_61             	(1L << (30-16))
#define RSTN_PWM_61             	(1L << (31-16))

#define GPIO_RSTN_63            	(rGPIO_BASE + 0x7f)  /*for PL1063*/
#define RSTN_INT_63             	(1L << (28-24))
#define RSTN_SCAN_63            	(1L << (30-24))
#define RSTN_KEY_63             	(1L << (30-24))
#define RSTN_PWM_63             	(1L << (31-24))
#define GPIO_KEY_MASK           	(rGPIO_BASE + 0x80)
#define GPIO_KEY_STATUS         	(rGPIO_BASE + 0x82)
#define GPIO_KEY_EDGE	         	(rGPIO_BASE + 0x84)

#define LAN_LED	  	 	 1
#define WIRELESS_LED	 	 2

#ifndef _EVM_BOARD_
#define GIO_LED_1		 0
#define GIO_LED_2 	 	 1
#define GIO_LED_3	 	 2
#else
#define GIO_OFFSET		 5
#define GIO_LED_1		 ( 0 + GIO_OFFSET)
#define GIO_LED_2 	 	 ( 1 + GIO_OFFSET)
#define GIO_LED_3	 	 ( 2 + GIO_OFFSET)
#endif
#define GIO_RESET_TP6842   	 3
#define GIO_PB_RESET	 	 4
#ifndef _EVM_BOARD_
#define GIO_REBOOT		 5
#else
#define GIO_REBOOT		 ( 4 + GIO_OFFSET)
#endif
#define GIO_RESET_I2C		 5	
#define GIO_I2C_CLK		 6
#define GIO_I2C_DATA		 7
#define GIO_IO1_IN		 8
#define GIO_IO2_IN		 9
#define GIO_IO1_OUT		10
#define GIO_IO2_OUT		11


typedef enum{
	GIO_DIR_OUT,
	GIO_DIR_IN,
	GIO_DIR_UNKNOWN
}gio_dir_t;

typedef enum{
	GIO_ANY_EDGE,
	GIO_RISING_EDGE,
	GIO_FALLING_EDGE
}gio_edge_t;


void unrequest_gio(unsigned char gpio);

unsigned char gio_get_dir(unsigned char gpio);
void gio_set_dir(unsigned char gpio, gio_dir_t dir);

unsigned char gio_get_oe(unsigned char gpio);
void gio_set_oe(unsigned char gpio, unsigned char bit);

unsigned char gio_get_inv(unsigned char gpio);
void gio_set_inv(unsigned char gpio, unsigned char bit);

unsigned char gio_get_input(unsigned char gpio);

unsigned char gio_get_output(unsigned char gpio);
void gio_set_output(unsigned char gpio, unsigned char bit);

unsigned char gio_get_irq_mask(unsigned char gpio);
void gio_set_irq_mask(unsigned char gpio, unsigned char bit);

unsigned char gio_get_irq_edge(unsigned char gpio);
void gio_set_irq_edge(unsigned char gpio, gio_edge_t edge);

unsigned char gio_get_irq_level(unsigned char gpio);
void gio_set_irq_level(unsigned char gpio, unsigned char bit);

unsigned char gio_get_irq_status(unsigned char gpio);
void gio_set_irq_status(unsigned char gpio, unsigned char bit);

void gio_set_in_func(unsigned char flags);

#endif
