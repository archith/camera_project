/**********************************************************************
 *
 * Filename:    UART_Utils.c
 *
 **********************************************************************/
#include "uart_regs.h"
#include "uart_utils.h"

/* PL1029 include file */
#include <asm/io.h>
#include <asm/atomic.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/arch-pl1029/pl_symbol_alarm.h>
#include <linux/major.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/hardware.h>

unsigned int pl_get_dev_hz(void);

// device clock is 16M Hz
const BaudRateClock AvailBaudRate16[] =
{
	{921600, 0, 0, 5574},
	{460800, 0, 1, 11150},
	{230400, 0, 3, 22300},
	{115200, 1, 3, 22300},
	{57600, 2, 3, 22300},
	{38400, 2, 5, 33450},
	{19200, 3, 5, 33450},
	{9600, 4, 5, 33450},
	{4800, 5, 5, 33450},
	{2400, 5, 12, 1364},
	{1200, 7, 5, 33450},
    {600, 8, 5, 33450},
    {300, 9, 5, 33450}
};

// device clock is 16.5M Hz
const BaudRateClock AvailBaudRate165[] =
{
	{921600, 0, 0, 7796},
	{460800, 0, 1, 15594},
	{230400, 0, 3, 31188},
	{115200, 1, 3, 31188},
	{57600, 2, 3, 31188},
	{38400, 2, 5, 46784},
	{19200, 3, 5, 46784},
	{9600, 4, 5, 46784},
	{4800, 5, 5, 46784},
	{2400, 5, 12, 28032},
	{1200, 7, 5, 46784},
    {600, 8, 5, 46784},
    {300, 9, 5, 46784}
};

// device clock is 30M Hz
const BaudRateClock AvailBaudRate30[] =
{
	{921600, 0, 1, 2260},
	{460800, 0, 3, 4522},
	{230400, 1, 3, 4522},
	{115200, 2, 3, 4522},
	{57600, 3, 3, 4522},
	{38400, 3, 5, 6784},
	{19200, 4, 5, 6784},
	{9600, 5, 5, 6784},
	{4800, 5, 11, 13568},
	{2400, 7, 5, 6784},
    {1200, 8, 5, 6784},
    {600, 9, 5, 6784},
    {300, 10, 5, 6784}
};

// device clock is 32M Hz
const BaudRateClock AvailBaudRate32[] =
{
	{921600, 0, 1, 11150},
	{460800, 0, 3, 22300},
	{230400, 1, 3, 22300},
	{115200, 2, 3, 22300},
	{57600, 3, 3, 22300},
	{38400, 3, 5, 33450},
	{19200, 4, 5, 33450},
	{9600, 5, 5, 33450},
	{4800, 5, 12, 1364},
	{2400, 7, 5, 33450},
    {1200, 8, 5, 33450},
    {600, 9, 5, 33450},
    {300, 10, 5, 33450}
};

// device clock is 33M Hz
const BaudRateClock AvailBaudRate33[] =
{
	{921600, 0, 1, 15594},
	{460800, 0, 3, 31188},
	{230400, 1, 3, 31188},
	{115200, 2, 3, 31188},
	{57600, 3, 3, 31188},
	{38400, 3, 5, 46784},
	{19200, 4, 5, 46784},
	{9600, 5, 5, 46784},
	{4800, 5, 12, 28032},
	{2400, 7, 5, 46784},
    {1200, 8, 5, 46784},
    {600, 9, 5, 46784},
    {300, 10, 5, 46784}
};


// device clock is 34.666M Hz
const BaudRateClock AvailBaudRate34[] =
{
	{921600, 0, 1, 22998},
	{460800, 0, 3, 45998},
	{230400, 1, 3, 45998},
	{115200, 2, 3, 45998},
	{57600, 3, 3, 45998},
	{38400, 3, 6, 3460},
	{19200, 4, 6, 3460},
	{9600, 5, 6, 3460},
	{4800, 5, 13, 6922},
	{2400, 7, 6, 3460},
    {1200, 8, 6, 3460},
    {600, 9, 6, 3460},
    {300, 10, 6, 3460}
};


// device clock is 40M Hz
const BaudRateClock AvailBaudRate40[] =
{
    {921600,  0,  1,  46704},
    {460800,  0,  4,  27874},
    {230400,  0,  9,  55750},
    {115200,  1,  9,  55750},
    { 57600,  2,  9,  55750},
    { 38400,  3,  7,   9044},
    { 19200,  4,  7,   9044},
    {  9600,  5,  7,   9044},
    {  4800,  6,  7,   9044},
    {  2400,  7,  7,   9044},
    {  1200,  8,  7,   9044},
    {   600,  9,  7,   9044},
    {   300, 10,  7,   9038},
};


// device clock is 48M Hz
const BaudRateClock AvailBaudRate48[] =
{
	{921600, 0, 2, 16724},
	{460800, 0,5, 33450},
	{230400, 1, 5, 33450},
	{115200, 2, 5, 33450},
	{57600, 3, 5, 33450},
	{38400, 4, 3, 57856},
	{19200, 5, 3, 57856},
	{9600, 5, 8, 50176},
	{4800, 7, 3, 57856},
	{2400, 8, 3, 57856},
    {1200, 9, 3, 57856},
    {600,10, 3, 57856},
    {300, 11, 3, 57856}
};

// device clock is 56M Hz
const BaudRateClock AvailBaudRate56[] =
{
	{921600, 0, 2, 52280},
	{460800, 0, 6, 39024},
	{230400, 1, 6, 39024},
	{115200, 2, 6, 39024},
	{57600, 3, 6, 39024},
	{38400, 4, 4, 45652},
	{19200, 5, 5, 45652},
	{9600, 5, 10, 25770},
	{4800, 7, 4, 45652},
	{2400, 8, 4, 45652},
    {1200, 9, 4, 45652},
    {600, 10, 4, 45652},
    {300, 11, 4, 45652}
};

// device clock is 66M Hz
const BaudRateClock AvailBaudRate66[] =
{
	{921600, 0, 3, 31188},
	{460800, 1, 3, 31188},
	{230400, 2, 3, 31188},
	{115200, 3, 3, 31188},
	{57600, 4, 3, 31188},
	{38400, 4, 5, 46784},
	{19200, 5, 5, 46784},
	{9600, 5, 12, 28032},
	{4800, 7, 5, 46784},
	{2400, 8, 5, 46784},
    {1200, 9, 5, 46784},
    {600, 10, 5, 46784},
    {300, 11, 5, 46784}
};

const BaudRateClock AvailBaudRate24[] =
{
    {921600, 0, 0, 41130},
    {460800, 0, 2, 16724},
    {230400, 0, 5, 33450},
    {115200, 1, 5, 33450},
    {57600, 2, 5, 33450},
    {38400, 3, 3, 57856},
    {19200, 4, 3, 57856},
    {9600, 5, 3, 57856},
    {4800, 5, 8, 50176},
    {2400, 7, 3, 57856},
    {1200, 8, 3, 57856},
    {600, 9, 3, 57856},
    {300, 10, 3, 57856},
};


void UART_SetBaudRate(volatile UART_REGS *baseAddr, UINT rate, UINT clk)
{
	int		idx, finalIdx = 0;
	UINT	finalRate = 0, regVal = 0;
	BOOL	bFound = FALSE;
    const BaudRateClock *AvailBaudRate;
    int numRate;

  if(!clk)
     clk = pl_get_dev_hz()/2/1000;
    //clk = readb(PL_CLK_UART)/1000;

	dbg("UART_SetBaudRate - clk: %d, rate: %d\n", clk, rate);

    //printk("UART baudrate clock is %d\n", clk);
    switch(clk) {

    case 16500:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate165;
      break;
    case 24000:
        AvailBaudRate = (BaudRateClock *)AvailBaudRate24;
        break;
    case 30000:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate30;
      break;
    case 32000:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate32;
      break;
    case 33000:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate33;
      break;
    case 34666:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate34;
      break;
    case 48000:
    case 60000:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate48;
      break;
    case 56000:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate56;
      break;
    case 66000:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate66;
      break;
    case 16000:
    default:
      AvailBaudRate = (BaudRateClock *)AvailBaudRate16;
      break;
    }

    numRate = sizeof(AvailBaudRate24) / sizeof(AvailBaudRate24[0]);

	for(idx = 0; idx < numRate; idx++) {
		if(rate == AvailBaudRate[idx].rate) {
			finalRate = rate;
			finalIdx = idx;
			bFound = TRUE;
			break;
		}
	}

	if(!bFound) {
		for(idx = 0; idx < numRate - 1; idx++) {
			if(rate > (AvailBaudRate[idx].rate + AvailBaudRate[idx + 1].rate) / 2) {
				finalRate = AvailBaudRate[idx].rate;
				finalIdx = idx;
				bFound = TRUE;
				break;
			} else if(rate < (AvailBaudRate[idx].rate + AvailBaudRate[idx + 1].rate) / 2 && rate > AvailBaudRate[idx + 1].rate) {
				finalRate = AvailBaudRate[idx + 1].rate;
				finalIdx = idx + 1;
				bFound = TRUE;
				break;
			}
		}
	}

	dbg("FinalRate found: %d - %d\n", bFound, finalRate);

	if(bFound) {
		regVal |= BRCR_E;
		regVal |= ((AvailBaudRate[finalIdx].BSEL << 24) & BRCR_BSEL);
		regVal |= ((AvailBaudRate[finalIdx].BDIV << 16) & BRCR_BDIV);
		regVal |= (AvailBaudRate[finalIdx].BCMP & BRCR_BCMP);

		SetBaudRateClockReg(baseAddr, regVal);
	} else {
		dbg("ERROR - UART_SetBaudRate FAILED!\n");
		;
	}
}

void UART_SetLineCoding(volatile UART_REGS *baseAddr, LineCodingStru *pLC)
{
	UINT	regVal = 0;

	dbg("UART_SetLineCoding - MS: 0x%X, Si: %d, So: %d, X: %d, L: %d, ISDV: %d, DSZ: 0x%X, PAR: 0x%X, STP: 0x%X\n",
        pLC->MS, pLC->Si, pLC->So, pLC->X, pLC->L, pLC->ISDV, pLC->DSZ, pLC->PAR, pLC->STP);

	regVal |= ((pLC->MS << 28) & LCR_MS);
    regVal |= ((pLC->Si << 17) & LCR_SI);
    regVal |= ((pLC->So << 16) & LCR_SO);
	regVal |= ((pLC->X << 15) & LCR_X);
	regVal |= ((pLC->L << 14) & LCR_L);
	regVal |= ((pLC->ISDV << 8) & LCR_ISDV);
	regVal |= ((pLC->DSZ << 5) & LCR_DSZ);
	regVal |= ((pLC->PAR << 2) & LCR_PAR);
	regVal |= (pLC->STP & LCR_STP);

	SetLineCodingReg(baseAddr, regVal);
}

void UART_GetLineCoding(volatile UART_REGS *baseAddr, LineCodingStru *pLC)
{
	UINT	regVal;

	regVal = GetLineCodingReg(baseAddr);

	pLC->MS = (regVal & LCR_MS) >> 28;
	pLC->X = (regVal & LCR_X) >> 15;
	pLC->L = (regVal & LCR_L) >> 14;
	pLC->ISDV = (regVal & LCR_ISDV) >> 8;
	pLC->DSZ = (regVal & LCR_DSZ) >> 5;
	pLC->PAR = (regVal & LCR_PAR) >> 2;
	pLC->STP = (regVal & LCR_STP);

	dbg("UART_GetLineCoding - MS: %d, X: %d, L: %d, ISDV: %d, DSZ: %d, PAR: %d, STP: %d\n", pLC->MS, pLC->X, pLC->L,
			pLC->ISDV, pLC->DSZ, pLC->PAR, pLC->STP);
}

void UART_SetXonXoffSymbol(volatile UART_REGS *baseAddr, UCHAR XonChar, UCHAR XoffChar)
{
	UINT	tmp, regVal = 0;

	dbg("UART_SetXonXoffSymbol - XonChar: 0x%02X, XoffChar: 0x%02X\n", XonChar, XoffChar);

	tmp = XoffChar;

	regVal |= ((tmp << 16) & XXSR_XOFF);
	regVal |= (XonChar & XXSR_XON);

	SetXonXoffSymbolReg(baseAddr, regVal);
}

void UART_SetInterruptControl(volatile UART_REGS *baseAddr, InterruptControl *control)
{
	UINT	tmp, regVal;

	dbg("UART_SetInterruptControl - RIDL: %d, C0: 0x%X, C1: 0x%X, R: %d, T: %d, E: %d\n", control->RIDL, control->C0,
			control->C1, control->R, control->T, control->E);

	regVal = GetInterruptControlStatusReg(baseAddr);

	regVal &= ~(ICSR_RIDL | ICSR_C0 | ICSR_C1 | ICSR_R | ICSR_T | ICSR_E);

	regVal |= (control->RIDL & ICSR_RIDL);

	tmp = control->C0;
	regVal |= ((tmp << 8) & ICSR_C0);

	tmp = control->C1;
	regVal |= ((tmp << 10) & ICSR_C1);

	tmp = control->R;
	regVal |= ((tmp << 13) & ICSR_R);

	tmp = control->T;
	regVal |= ((tmp << 14) & ICSR_T);

	tmp = control->E;
	regVal |= ((tmp << 15) & ICSR_E);

	SetInterruptControlStatusReg(baseAddr, regVal);
}

void UART_GetInterruptStatus(volatile UART_REGS *baseAddr, InterruptStatus *status)
{
	UINT	regVal;

	regVal = GetInterruptControlStatusReg(baseAddr);

	status->Z0 = (regVal & ICSR_Z0) >> 24;
	status->Z1 = (regVal & ICSR_Z1) >> 26;
	status->Ri = (regVal & ICSR_Ri) >> 29;
	status->Ti = (regVal & ICSR_Ti) >> 30;
	status->I = (regVal & ICSR_I) >> 31;

	dbg("UART_GetInterruptStatus - Z0: %d, Z1: %d, Ri: %d, Ti: %d, I: %d\n", status->Z0, status->Z1, status->Ri,
			status->Ti, status->I);
}

void UART_GetTransmitterStatus(volatile UART_REGS *baseAddr, UCHAR *status)
{
	UINT	tmp, tmpStatus;

	tmp = GetTransmitterCommandStatusReg(baseAddr);
	tmpStatus = (tmp & (TCSR_F | TCSR_E | TCSR_K | TCSR_B)) >> 12;
	*status = (UCHAR) tmpStatus;

	dbg("UART_GetTransmitterStatus - status: 0x%X\n", *status);
}

void UART_SetTransmitterCommand(volatile UART_REGS *baseAddr, UCHAR cmd)
{
	UINT	tmp, regVal;

	dbg("UART_SetTransmitterCommand - cmd: 0x%X\n", cmd);

	regVal = GetTransmitterCommandStatusReg(baseAddr);
	regVal &= ~TCSR_CMD;

	tmp = cmd;

	regVal |= ((tmp << 28) & TCSR_CMD);

	SetTransmitterCommandStatusReg(baseAddr, regVal);
}

/************************************************************
	UART_SetTransmitterData -

		count - DMA Data Counter (fill zero if DMA is not used)
		data	- data byte (DMA is used when zero)
*************************************************************/
void UART_SetTransmitterData(volatile UART_REGS *baseAddr, UINT count, UCHAR data)
{
	UINT	regVal;

	dbg("UART_SetTransmitterData - count: %d, data: 0x%02X\n", count, data);

	regVal = GetTransmitterDataCountReg(baseAddr);

	regVal &= ~TDCR_DATA;
	regVal |= (data & TDCR_DATA);

	if(count) {
		regVal &= ~TDCR_TXDCNT;
		regVal |= ((count << 16) & TDCR_TXDCNT);
	}

	SetTransmitterDataCountReg(baseAddr, regVal);
}

void UART_GetTransmitterDMAStatus(volatile UART_REGS *baseAddr, TransmitterDMAStatus *status)
{
	UINT	regVal;

	regVal = GetTransmitterDataCountReg(baseAddr);

	status->ST = (regVal & TDCR_ST) >> 30;
	status->TXDCNT = (regVal & TDCR_TXDCNT) >> 16;
	status->ERR = (regVal & TDCR_ERR) >> 14;

	dbg("UART_GetTransmitterStatus - ST: %d, TXDCNT: %d, ERR: %d\n", status->ST, status->TXDCNT, status->ERR);
}

void UART_SetReceiverCommand(volatile UART_REGS *baseAddr, UCHAR cmd)
{
	UINT	tmp, regVal;

	dbg("UART_SetReceiverCommand - cmd: 0x%X\n", cmd);

	regVal = GetReceiverCommandStatusReg(baseAddr);
	regVal &= ~RCSR_CMD;

	tmp = cmd;

	regVal |= ((tmp << 28) & RCSR_CMD);

	SetReceiverCommandStatusReg(baseAddr, regVal);
}

void UART_GetReceiverData(volatile UART_REGS *baseAddr, UCHAR *rdata)
{
	UINT	regVal;


	regVal = GetReceiverCommandStatusReg(baseAddr);
	//printk("UART_GetReceiverData - data: 0x%X\n", regVal);
	dbg("UART_GetReceiverData - data: 0x%X\n", regVal & 0xff);
	*rdata = (regVal & 0xff);
}


void UART_GetReceiverStatusData(volatile UART_REGS *baseAddr, ReceiverStatusData *statusData)
{
	UINT	regVal;

	regVal = GetReceiverCommandStatusReg(baseAddr);

	statusData->DATA = (regVal & RCSR_DATA);
	statusData->V = (regVal & RCSR_V) >> 8;
	statusData->Sb = (regVal & RCSR_Sb) >> 11;
	statusData->B = (regVal & RCSR_B) >> 12;
	statusData->Pe = (regVal & RCSR_Pe) >> 13;
	statusData->Fe = (regVal & RCSR_Fe) >> 14;
	statusData->Oe = (regVal & RCSR_Oe) >> 15;

	dbg("UART_GetReceiverStatusData - DATA: 0x%02X, V: %d, Sb: %d, B: %d, Pe: %d, Fe: %d, Oe: %d\n", statusData->DATA,
			statusData->V, statusData->Sb, statusData->B, statusData->Pe, statusData->Fe, statusData->Oe);
}

void UART_SetReceiverDMAWaterMark(volatile UART_REGS *baseAddr, UINT mark)
{
	UINT	regVal = 0;

	dbg("UART_SetReceiverDMAWaterMark - mark: %d[0x%03X]\n", mark, mark);

	/*
	regVal = GetReceiverDMAControlStatusReg();
	regVal &= ~RDCSR_HWM;
    */
	regVal |= ((mark << 16) & RDCSR_HWM);

	SetReceiverDMAControlStatusReg(baseAddr, regVal);
}

void UART_GetReceiverDMAStatus(volatile UART_REGS *baseAddr, ReceiverDMAStatus *status)
{
	UINT	regVal;

	regVal = GetReceiverDMAControlStatusReg(baseAddr);

	status->RC = (regVal & RDCSR_RC);
	status->Pe = (regVal & RDCSR_Pe) >> 13;
	status->Fe = (regVal & RDCSR_Fe) >> 14;
	status->Oe = (regVal & RDCSR_Oe) >> 15;
	status->H = (regVal & RDCSR_H) >> 28;
	status->ST = (regVal & RDCSR_ST) >> 30;

	dbg("UART_GetReceiverDMAStatus - RC: %d, Pe: %d, Fe: %d, Oe: %d, H: %d, ST: 0x%X\n", status->RC, status->Pe,
			status->Fe, status->Oe, status->H, status->ST);
}

void UART_SetACIOControl(volatile UART_REGS *baseAddr, int idx, UCHAR mode, UCHAR source)
{
	UINT	regVal;

	dbg("UART_SetACIOControl - idx: %d, source: 0x%X, mode: 0x%X\n", idx, source, mode);

	if(idx == 0) {
		regVal = GetACIO0ControlStatusReg(baseAddr);

		regVal &= ~A0CSR_S;
		regVal &= ~A0CSR_M;

		regVal |= (source & A0CSR_S);
		regVal |= ((mode << 3) & A0CSR_M);

		SetACIO0ControlStatusReg(baseAddr, regVal);
	} else {
		regVal = GetACIO1ControlStatusReg(baseAddr);

		regVal &= ~A1CSR_S;
		regVal &= ~A1CSR_M;

		regVal |= (source & A1CSR_S);
		regVal |= ((mode << 3) & A1CSR_M);

		SetACIO1ControlStatusReg(baseAddr, regVal);
	}
}

void UART_GetACIOStatus(volatile UART_REGS *baseAddr, int idx, ACIOStatus *status)
{
	UINT	regVal;

	if(idx == 0) {
		regVal = GetACIO0ControlStatusReg(baseAddr);

		status->S = (regVal & A0CSR_S);
		status->V = (regVal & A0CSR_V) >> 16;
	} else {
		regVal = GetACIO1ControlStatusReg(baseAddr);

		status->S = (regVal & A1CSR_S);
		status->V = (regVal & A1CSR_V) >> 16;
	}

	dbg("UART_GetACIOStatus - idx: %d, S: 0x%X, V: %d\n", idx, status->S, status->V);
}
