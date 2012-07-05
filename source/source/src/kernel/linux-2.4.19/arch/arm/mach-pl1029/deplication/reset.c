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

void (*back_to_prom)(void) = (void (*)(void))0x00000000;

void pl_machine_restart(char *command)
{
    int s=0;
//JIM-CHECK !!!     
#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
#else
    s = ST0_BEV; /* Enable prom exception handling */
    write_32bit_cp0_register(CP0_STATUS, s);
    asm("nop;break;nop;");
#endif
    back_to_prom();
}

void pl_machine_halt(void)
{
    int s=0;
#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
#else
    s = ST0_BEV; /* Enable prom exception handling */
    write_32bit_cp0_register(CP0_STATUS, s);
    asm("nop;break;nop;");
#endif
    back_to_prom();
}

void pl_machine_power_off(void)
{
    int s=0;
#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
#else
    s = ST0_BEV; /* Enable prom exception handling */
    write_32bit_cp0_register(CP0_STATUS, s);
    asm("nop;break;nop;");
#endif
    back_to_prom();
}
