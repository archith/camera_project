/*
 *  linux/include/asm-arm/hardware/serial_famba.h
 *
 *  Internal header file for AMBA serial ports
 *
 *  Copyright (C) ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
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
#ifndef ASM_ARM_HARDWARE_SERIAL_FAMBA_H
#define ASM_ARM_HARDWARE_SERIAL_FAMBA_H

/* -------------------------------------------------------------------------------
 *  From Faraday AMBA UART (PL010) Block Specification (ARM-0001-CUST-DSPC-A03)
 * -------------------------------------------------------------------------------
 *  UART Register Offsets.
 */
//include flib

#include "asm/hardware/flib/serial.h"
#include <asm/arch/flib/cpe.h>

//#define AMBA_UARTRSR_ANY	(AMBA_UARTRSR_OE|AMBA_UARTRSR_BE|AMBA_UARTRSR_PE|AMBA_UARTRSR_FE)
//#define AMBA_UARTFR_MODEM_ANY	(AMBA_UARTFR_DCD|AMBA_UARTFR_DSR|AMBA_UARTFR_CTS)
#endif
