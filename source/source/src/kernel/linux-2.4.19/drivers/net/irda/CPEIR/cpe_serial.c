/****************************************************************************
* Porting to Linux on 20030822											   *
* Author: Paul Chiang													   *
* Version: 0.1								  							   * 
* History: 								  								   *
*          0.1 new creation						   						   *
* Todo: 																   *
****************************************************************************/
/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name:serial.c                                                            *
* Description: serial library routine                                      *
* Author: Fred Chien                                                       *
****************************************************************************/

//#include <stdio.h>
//#include <Stdlib.h>

//#include "fLib.h"
//#include "except.h"
//#include "cpe.h"
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/arch-cpe/hardware.h>
#include "cpe_serial.h"

//UINT32 DebugSerialPort = CPE_UART1_BASE;
//UINT32 SystemSerialPort = CPE_UART2_BASE;
//----------------------------------
void fLib_SetSerialMode(UINT32 port, UINT32 mode)
{
	UINT32 mdr;
	
    mdr = INW(port + SERIAL_MDR);
    mdr &= ~SERIAL_MDR_MODE_SEL;
    OUTW(port + SERIAL_MDR, mdr | mode);
}

//----------------------------------
void fLib_EnableIRMode(UINT32 port, UINT32 TxEnable, UINT32 RxEnable)
{
	UINT32 acr;
	
    acr = INW(port + SERIAL_ACR);
    acr &= ~(SERIAL_ACR_TXENABLE | SERIAL_ACR_RXENABLE);
    if(TxEnable)
    	acr |= SERIAL_ACR_TXENABLE;
    if(RxEnable)
    	acr |= SERIAL_ACR_RXENABLE;
    OUTW(port + SERIAL_ACR, acr);
}

/*****************************************************************************/
#if 0
void fLib_SerialInit(UINT32 port, UINT32 baudrate, UINT32 parity,UINT32 num,UINT32 len)
{
	UINT32 lcr;
	
    lcr = INW(port + SERIAL_LCR) & ~SERIAL_LCR_DLAB;
	/* Set DLAB=1 */
    OUTW(port + SERIAL_LCR,SERIAL_LCR_DLAB);
    /* Set baud rate */
    OUTW(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
    OUTW(port + SERIAL_DLL, (baudrate & 0xff));

	//clear orignal parity setting
	lcr &= 0xc0;
	
	switch (parity)
	{
		case PARITY_NONE:	
			//do nothing
    		break;
    	case PARITY_ODD:
		    lcr|=SERIAL_LCR_ODD;
   		 	break;
    	case PARITY_EVEN:
    		lcr|=SERIAL_LCR_EVEN;
    		break;
    	case PARITY_MARK:
    		lcr|=(SERIAL_LCR_STICKPARITY|SERIAL_LCR_ODD);
    		break;
    	case PARITY_SPACE:
    		lcr|=(SERIAL_LCR_STICKPARITY|SERIAL_LCR_EVEN);
    		break;
    
    	default:
    		break;
    }
    
    if(num==2)
		lcr|=SERIAL_LCR_STOP;
	
	len-=5;
	
	lcr|=len;	
    
    OUTW(port+SERIAL_LCR,lcr);    
}
#endif
//----------------------------------
/* duplicate function 2003-5-23, dragon
void fLib_SetSerialLoopback(UINT32 port, UINT32 onoff)
{
	UINT32 temp;

	temp=INW(port+SERIAL_MCR);
	if(onoff==ON)	
		temp|=SERIAL_MCR_LPBK;
	else
		temp&=~(SERIAL_MCR_LPBK);
		
	OUTW(port+SERIAL_MCR,temp);	
}
*/ 
//----------------------------------
void fLib_SetSerialFifoCtrl(UINT32 port, UINT32 level_tx, UINT32 level_rx, UINT32 resettx, UINT32 resetrx)  //V1.20//ADA10022002
{
	UINT8 fcr = 0;
 
 	fcr |= SERIAL_FCR_FE;
 
 	switch(level_rx)  //V1.20//ADA10022002//Start
 	{
 		case 4:
 			fcr|=0x40;
 			break;
 		case 8:
 			fcr|=0x80;
 			break;
 		case 14:
 			fcr|=0xc0;
 			break;
 		default:
 			break;
 	}
  //V1.20//ADA10022002//Start 
  //dragon 2003.5.22
 	switch(level_tx)
 	{
 		case 3:
 			fcr|=0x01<<4;
 			break;
 		case 9:
 			fcr|=0x02<<4;
 			break;
 		case 13:
 			fcr|=0x03<<4;
 			break;
 		default:
 			break;
 	}
  //V1.20//ADA10022002//End 	
	if(resettx)
		fcr|=SERIAL_FCR_TXFR;

	if(resetrx)
		fcr|=SERIAL_FCR_RXFR; 	

	OUTW(port+SERIAL_FCR,fcr);
}

//----------------------------------
void fLib_DisableSerialFifo(UINT32 port)
{
	OUTW(port+SERIAL_FCR,0);
}
//----------------------------------

void fLib_SetSerialInt(UINT32 port, UINT32 IntMask)
{
	OUTW(port + SERIAL_IER, IntMask);
}
//--------------------------------------paulong 20030826
void fLib_ResetSerialInt(UINT32 port)
{
	OUTW(port + SERIAL_IER, 0);
}

//----------------------------------
char fLib_GetSerialChar(UINT32 port)
{   
    char Ch;    
	UINT32 status;
	
   	do
	{
	 	status=INW(port+SERIAL_LSR);
	}
	while (!((status & SERIAL_LSR_DR)==SERIAL_LSR_DR));	// wait until Rx ready
    Ch = INW(port + SERIAL_RBR);    
    return (Ch);
}				
//----------------------------------
void fLib_PutSerialChar(UINT32 port, char Ch)
{
  	UINT32 status;
  
    do
	{
	 	status=INW(port+SERIAL_LSR);
	}while (!((status & SERIAL_LSR_THRE)==SERIAL_LSR_THRE));	// wait until Tx ready	   
    OUTW(port + SERIAL_THR,Ch);
} 

//----------------------------------
void fLib_WriteSerialChar(UINT32 port, char Ch)
{
  	
    OUTW(port + SERIAL_THR,Ch);
}

//----------------------------------
void fLib_PutSerialStr(UINT32 port, char *Str)
{
  	char *cp;
   
 	for(cp = Str; *cp != 0; cp++)       
   		fLib_PutSerialChar(port, *cp);	
}

//----------------------------------



void fLib_Modem_waitcall(UINT32 port)
{
	fLib_PutSerialStr(port, "ATS0=2\r");	
}					
//----------------------------------
void fLib_Modem_call(UINT32 port, char *tel)
{
	fLib_PutSerialStr(port, "ATDT");
	fLib_PutSerialStr(port,  tel);
	fLib_PutSerialStr(port, "\r");
}
/*
//----------------------------------
int fLib_Modem_getchar(UINT32 port,int TIMEOUT)
{
  	UINT64 start_time, middle_time, dead_time;
  	UINT32 status;
	INT8 ch;
	UINT32 n=0;

  	start_time = fLib_CurrentT1Tick();
  	dead_time = start_time + TIMEOUT;
  
 	do
	{			
		if(n>1000)
		{
			middle_time = fLib_CurrentT1Tick();
			if (middle_time > dead_time)
				return 0x100;
		}			
		status = INW(port + SERIAL_LSR);	    
		n++;
	}while (!((status & SERIAL_LSR_DR)==SERIAL_LSR_DR));	
		  
    ch = INW(port + SERIAL_RBR);    
    return (ch);
}*/
//----------------------------------
/*
BOOL fLib_Modem_putchar(UINT32 port, INT8 Ch)
{
	UINT64 start_time, middle_time, dead_time;
  	UINT32 status;
	UINT32 n=0;
	  
  	start_time = fLib_CurrentT1Tick();
  	dead_time = start_time + 5;
  	  
	do
	{
		if(n>1000)
		{
			middle_time = fLib_CurrentT1Tick();
			if (middle_time > dead_time)
				return FALSE;
		}	
		status = INW(port + SERIAL_LSR);	    
		n++;
	}while (!((status & SERIAL_LSR_THRE)==SERIAL_LSR_THRE));	
	  
	OUTW(port + SERIAL_THR, Ch);
	    
	return TRUE;
}*/
//----------------------------------
void fLib_EnableSerialInt(UINT32 port, UINT32 mode)
{
UINT32 data;

	data = INW(port + SERIAL_IER);
	OUTW(port + SERIAL_IER, data | mode);
}
//----------------------------------

void fLib_DisableSerialInt(UINT32 port, UINT32 mode)
{
UINT32 data;

	data = INW(port + SERIAL_IER);
	mode = data & (~mode);	
	OUTW(port + SERIAL_IER, mode);
}
//----------------------------------
UINT32 fLib_SerialIntIdentification(UINT32 port)
{
	return INW(port + SERIAL_IIR);
}
//----------------------------------
void fLib_SetSerialLineBreak(UINT32 port)
{
UINT32 data;

	data = INW(port + SERIAL_LCR);
	OUTW(port + SERIAL_LCR, data | SERIAL_LCR_SETBREAK);
}
//----------------------------------
void fLib_SetSerialLoopBack(UINT32 port,UINT32 onoff)
{
UINT32 temp;

	temp = INW(port+SERIAL_MCR);
	if(onoff == ON)	
		temp |= SERIAL_MCR_LPBK;
	else
		temp &= ~(SERIAL_MCR_LPBK);
		
	OUTW(port+SERIAL_MCR,temp);	
}
//----------------------------------
void fLib_SerialRequestToSend(UINT32 port)
{
UINT32 data;

	data = INW(port + SERIAL_MCR);
	OUTW(port + SERIAL_MCR, data | SERIAL_MCR_RTS);
}
//----------------------------------
void fLib_SerialStopToSend(UINT32 port)
{
UINT32 data;

	data = INW(port + SERIAL_MCR);
	data &= ~(SERIAL_MCR_RTS);	
	OUTW(port + SERIAL_MCR, data);
}
//----------------------------------
void fLib_SerialDataTerminalReady(UINT32 port)
{
UINT32 data;

	data = INW(port + SERIAL_MCR);
	OUTW(port + SERIAL_MCR, data | SERIAL_MCR_DTR);
}
//----------------------------------
void fLib_SerialDataTerminalNotReady(UINT32 port)
{
UINT32 data;

	data = INW(port + SERIAL_MCR);
	data &= ~(SERIAL_MCR_DTR);	
	OUTW(port + SERIAL_MCR, data);
}
//----------------------------------
UINT32 fLib_ReadSerialLineStatus(UINT32 port)
{
	return INW(port + SERIAL_LSR);
}
//----------------------------------
UINT32 fLib_ReadSerialModemStatus(UINT32 port)
{
	return INW(port + SERIAL_MSR);
}   

//----------------------------------
void fLib_EnableMCROuts(UINT32 port, UINT32 index)
{
    UINT32 data, value;

	data = INW(port + SERIAL_MCR);
    
    switch(index)
    {
       case 1:
            value = 1 << 2;  
            break;
       case 2:
            value = 1 << 3;  
            break;
       case 3:
            value = 1 << 5;  
            break;
       default:
            value = 0;
    }

	data |= value;	
	OUTW(port + SERIAL_MCR, data);
}

void fLib_DisableMCROuts(UINT32 port, UINT32 index)
{ 
    UINT32 data, value;

	data = INW(port + SERIAL_MCR);
    
    switch(index)
    {
       case 1:
            value = 1 << 2;  
            break;
       case 2:
            value = 1 << 3;  
            break;
       case 3:
            value = 1 << 5;  
            break;
       default:
            value = 0;
    }

	data &= (~value);	
	OUTW(port + SERIAL_MCR, data);
}


// End of file - serial.c

