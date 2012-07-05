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
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/kbd_kern.h>
#include <linux/keyboard.h>

struct tty_struct *console_tty = NULL;

void put_queue(int ch)
{
    struct tty_struct *tty = console_tty;

	if (tty) {
		tty_insert_flip_char(tty, ch, 0);
		con_schedule_flip(tty);
	}
}


/*
 * This routine is the bottom half of the keyboard interrupt
 * routine, and runs with all interrupts enabled. It does
 * console changing, led setting and copy_to_cooked, which can
 * take a reasonably long time.
 */
static void kbd_bh(unsigned long dummy)
{
    /* do nothing */
}
DECLARE_TASKLET_DISABLED(keyboard_tasklet, kbd_bh, 0);

void handle_scancode(unsigned char scancode, int down)
{
	unsigned char keycode;
    u_short keysym;

    if (down) {
    	keycode = scancode;
        keysym = key_maps[0][keycode];
        put_queue(keysym);
    }
}
