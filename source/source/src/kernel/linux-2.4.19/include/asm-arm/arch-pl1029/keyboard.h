/*
 *  linux/include/asm-arm/arch-ti926/keyboard.h
 *
 *  Copyright (C) 2000 RidgeRun, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Keyboard driver definitions for ARM
 */
#ifndef __ASM_ARM_ARCH_KEYBOARD_H
#define  __ASM_ARM_ARCH_KEYBOARD_H

#define kbd_init_hw()       do { } while(0)
#define kbd_enable_irq()    do { } while(0)
#define kbd_disable_irq()   do { } while(0)


#if 0
#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
#define DISABLE_KBD_DURING_INTERRUPTS 0

extern int pckbd_setkeycode(unsigned int scancode, unsigned int keycode);
extern int pckbd_getkeycode(unsigned int scancode);
extern int pckbd_translate(unsigned char scancode, unsigned char *keycode,
			   char raw_mode);
extern char pckbd_unexpected_up(unsigned char keycode);
extern void pckbd_leds(unsigned char leds);
extern void pckbd_init_hw(void);
extern unsigned char pckbd_sysrq_xlate[128];
extern void kbd_forward_char (int ch);

#define kbd_setkeycode		pckbd_setkeycode
#define kbd_getkeycode		pckbd_getkeycode
#define kbd_translate		pckbd_translate
#define kbd_unexpected_up	pckbd_unexpected_up
#define kbd_leds		pckbd_leds
#define kbd_init_hw		pckbd_init_hw
#define kbd_sysrq_xlate         pckbd_sysrq_xlate

#define SYSRQ_KEY 0x54

/* Some stoneage hardware needs delays after some operations.  */
#define kbd_pause() do { } while(0)

struct kbd_ops {
	/* Keyboard driver resource allocation  */
	void (*kbd_request_region)(void);
	int (*kbd_request_irq)(void (*handler)(int, void *, struct pt_regs *));

	/* PSaux driver resource management  */
	int (*aux_request_irq)(void (*handler)(int, void *, struct pt_regs *));
	void (*aux_free_irq)(void);

	/* Methods to access the keyboard processor's I/O registers  */
	unsigned char (*kbd_read_input)(void);
	void (*kbd_write_output)(unsigned char val);
	void (*kbd_write_command)(unsigned char val);
	unsigned char (*kbd_read_status)(void);
};

extern struct kbd_ops *kbd_ops;

/* Do the actual calls via kbd_ops vector  */
#define kbd_request_region() kbd_ops->kbd_request_region()
#define kbd_request_irq(handler) kbd_ops->kbd_request_irq(handler)

#define aux_request_irq(hand, dev_id) kbd_ops->aux_request_irq(hand)
#define aux_free_irq(dev_id) kbd_ops->aux_free_irq()

#define kbd_read_input() kbd_ops->kbd_read_input()
#define kbd_write_output(val) kbd_ops->kbd_write_output(val)
#define kbd_write_command(val) kbd_ops->kbd_write_command(val)
#define kbd_read_status() kbd_ops->kbd_read_status()

#else
/*
 * Required by drivers/char/keyboard.c. I took these from arch-arc.
 * --gmcnutt
 */
#define kbd_setkeycode(sc,kc) (-EINVAL)
#define kbd_getkeycode(sc) (-EINVAL)
#define kbd_translate(sc,kcp,rm) ({ *(kcp) = (sc); 1; })
#define kbd_unexpected_up(kc) (0200)
#define kbd_leds(leds)
#define kbd_init_hw()
#define kbd_enable_irq() /* what irq? ;) --gmcnutt */
#define kbd_disable_irq()

#endif

#endif
#endif /* __ASM_ARM_ARCH_KEYBOARD_H */
