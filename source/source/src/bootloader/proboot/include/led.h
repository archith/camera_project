#ifndef __LED_H__
#define __LED_H__
/*
#define GPIO_REG_BASE	0x194c0000
#define GPIO_REG_OE	(GPIO_REG_BASE + 0x54)
#define GPIO_REG_LED	(GPIO_REG_BASE + 0x56)
#define GPIO_REG_OD	(GPIO_REG_BASE + 0x58)
*/
#define IO_DBG_PORT	0x1B800080

void led_show (unsigned char val);

#endif // __LED_H__
