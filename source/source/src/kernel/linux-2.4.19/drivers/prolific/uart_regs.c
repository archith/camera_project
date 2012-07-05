/**********************************************************************
 *
 * Filename:    UART_Regs.c
 *
 **********************************************************************/
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "uart_regs.h"

// Baud Rate Clock Register Functions
void SetBaudRateClockReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetBaudRateClockReg - val: 0x%08X\n", val);

	pRegs->BaudRateClock = val;
}

UINT GetBaudRateClockReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->BaudRateClock;

	dbg("GetBaudRateClockReg - val: 0x%08X\n", val);

	return val;
}

// Line Coding Register Functions
void SetLineCodingReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetLineCodingReg - val: 0x%08X\n", val);

	pRegs->LineCoding = val;
}

UINT GetLineCodingReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->LineCoding;

	dbg("GetLineCodingReg - val: 0x%08X\n", val);

	return val;
}

// Xon/Xoff Symbol Register Functions
void SetXonXoffSymbolReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetXonXoffSymbolReg - val: 0x%08X\n", val);

	pRegs->XonXoffSymbol = val;
}

UINT GetXonXoffSymbolReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->XonXoffSymbol;

	dbg("GetXonXoffSymbolReg - val: 0x%08X\n", val);

	return val;
}

// Interrupt Control/Status Register Functions
void SetInterruptControlStatusReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetInterruptControlStatusReg - val: 0x%08X\n", val);

	pRegs->InterruptControlStatus = val;
}

UINT GetInterruptControlStatusReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->InterruptControlStatus;

	dbg("GetInterruptControlStatusReg - val: 0x%08X\n", val);

	return val;
}

// Transmitter Command/Status Register Functions
void SetTransmitterCommandStatusReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetTransmitterCommandStatusReg - val: 0x%08X\n", val);

	pRegs->TransmitterCommandStatus = val;
}

UINT GetTransmitterCommandStatusReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->TransmitterCommandStatus;

	dbg("GetTransmitterCommandStatusReg - val: 0x%08X\n", val);

	return val;
}

// Transmitter Data/Count Register Functions
void SetTransmitterDataCountReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetTransmitterDataCountReg - val: 0x%08X\n", val);

	pRegs->TransmitterDataCount = val;
}

UINT GetTransmitterDataCountReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->TransmitterDataCount;

	dbg("GetTransmitterDataCountReg - val: 0x%08X\n", val);

	return val;
}

// Receiver Command/Status Register Functions
void SetReceiverCommandStatusReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetReceiverCommandStatusReg - val: 0x%08X\n", val);

	pRegs->ReceiverCommandStatus = val;
}

UINT GetReceiverCommandStatusReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->ReceiverCommandStatus;

	dbg("GetReceiverCommandStatusReg - val: 0x%08X\n", val);

	return val;
}

// Receiver DMA Control/Status Register Functions
void SetReceiverDMAControlStatusReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetReceiverDMAControlStatusReg - val: 0x%08X\n", val);

	pRegs->ReceiverDMAControlStatus = val;
}

UINT GetReceiverDMAControlStatusReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->ReceiverDMAControlStatus;

	dbg("GetReceiverDMAControlStatusReg - val: 0x%08X\n", val);

	return val;
}

// ACIO0 Control/Status Register Functions
void SetACIO0ControlStatusReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetACIO0ControlStatusReg - val: 0x%08X\n", val);

	pRegs->ACIO0ControlStatus = val;
}

UINT GetACIO0ControlStatusReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->ACIO0ControlStatus;

	dbg("GetACIO0ControlStatusReg - val: 0x%08X\n", val);

	return val;
}

// ACIO1 Control/Status Register Functions
void SetACIO1ControlStatusReg(volatile UART_REGS *pRegs, UINT val)
{
	dbg("SetACIO1ControlStatusReg - val: 0x%08X\n", val);

	pRegs->ACIO1ControlStatus = val;
}

UINT GetACIO1ControlStatusReg(volatile UART_REGS *pRegs)
{
	UINT	val = pRegs->ACIO1ControlStatus;

	dbg("GetACIO1ControlStatusReg - val: 0x%08X\n", val);

	return val;
}
