/****************************************************************************
* Porting to Linux on 20030822											   *
* Author: Paul Chiang													   *
* Version: 0.1								  							   * 
* History: 								  								   *
*          0.1 new creation						   						   *
* Todo: 																   *
****************************************************************************/
//#include <stdio.h>
//#include <Stdlib.h>
//#include <string.h>
#include <linux/kernel.h>
#include <asm/io.h>
//#include "fLib.h"
#include "cpe_serial.h"  
#include "irda.h"
//#include "command.h"
//#include "except.h"  
//#include "APB_DMA.h"
//#include "fa510.h"

UINT8 IRDA_SIP_TimerCounter;
UINT32 IRDA_PORT_ForSIP;
IRDA_FIR_TX_Config_Status                     IrDA_TX_Status;
IRDA_FIR_TX_Data_Buffer_Status                SendData;
IRDA_FIR_RX_Config_Status                     IrDA_RX_Status;
IRDA_FIR_RX_Data_Buffer_Status                ReceiveData;
FIR_TEST_SELECT_PORT_STRUCTURE                SelectPort;
UINT8  ReceiveBufferDMA[RX_BUFFER_SIZE];
IRDA_SIR_RX_Status                            SIR_RX_Device;
IRDA_SIR_TX_Status                            SIR_TX_Device;

//=========================================================================================
// Function Name: fLib_IRDA_INT_Init
// Description: 
//=========================================================================================
/*int fLib_IRDA_INT_Init(UINT32 IRQ_IrDA,PrHandler function)
{
	//setup INT
	fLib_ClearInt(IRQ_IrDA);
	fLib_CloseInt(IRQ_IrDA);
	
	//connnect IrDA interrupt
	if (!fLib_ConnectInt(IRQ_IrDA, function ))
	{
		return FALSE;
	}		
	
	//enable interrupt
	fLib_SetIntTrig(IRQ_IrDA,LEVEL,H_ACTIVE);
	fLib_EnableInt(IRQ_IrDA);	
	
	return TRUE;
}

//=========================================================================================
// Function Name: fLib_IRDA_INT_Init
// Description: 
//=========================================================================================
int fLib_IRDA_INT_Disable(UINT32 IRQ_IrDA)
{
	
 if (fLib_CloseInt(IRQ_IrDA)==TRUE)
    return TRUE;
 else return FALSE;   
 
}	*/
//=========================================================================================
// Function Name: fLib_SetFIRPrescal =>
// Description: "DLL=0x00" "DLM=0x04" "PSR=0x08"
//=========================================================================================				
void fLib_SetFIRPrescal(UINT32 port,UINT32 PSR_Value)
{
	//see IrDA spec P.33
	
	UINT32 lcr;	
    lcr = INW(port + SERIAL_LCR) & ~SERIAL_LCR_DLAB;
	/* Set DLAB=1 */
    OUTW(port + SERIAL_LCR, SERIAL_LCR_DLAB);	
	
	if(PSR_Value>0x1f)
	{
		PSR_Value=1;
	}
	OUTW(port + SERIAL_PSR, PSR_Value);
	
    OUTW(port + SERIAL_DLM, 0);
    OUTW(port + SERIAL_DLL, 1); 
    
    OUTW(port + SERIAL_LCR,lcr);		
} 

//=========================================================================================
// Function Name: fLib_IRDA_Set_FCR / fLib_IRDA_Enable_FCR / fLib_IRDA_Disable_FCR fLib_IRDA_Set_FCR_Trigl_Level
//
// Description: Register "FCR=0x08"
//=========================================================================================	
void fLib_IRDA_Set_FCR(UINT32 port, UINT32 setmode)
{

  OUTW(port+SERIAL_FCR,setmode);

}
//------------------------------------
void fLib_IRDA_Enable_FCR(UINT32 port, UINT32 setmode)
{
    UINT8 data;
    
    data=0x01; //FIFO Enable
    OUTW(port+SERIAL_FCR,(data|setmode));

}	
//------------------------------------						
void fLib_IRDA_Set_FCR_Trigl_Level(UINT32 port, UINT32 Tx_Tri_Level , UINT32 Rx_Tri_Level )
{
    UINT8 data;
    
    data=0x01; //FIFO Enable
    
    switch(Tx_Tri_Level)
          {
           case SERIAL_FCR_TX_TRI1:
                data = data | 0x00;                
           break;
           case SERIAL_FCR_TX_TRI2:
                data = data | 0x10;                
           break;    
           case SERIAL_FCR_TX_TRI3:
                data = data | 0x20;                
           break;
           case SERIAL_FCR_TX_TRI4:
                data = data | 0x30;                
           break;   
           default:
                data = data | 0x00;  
           break;           
            
           }
    
    switch(Rx_Tri_Level)
          {
           case SERIAL_FCR_RX_TRI1:
                data = data | 0x00;                
           break;
           case SERIAL_FCR_RX_TRI2:
                data = data | 0x40;                
           break;    
           case SERIAL_FCR_RX_TRI3:
                data = data | 0x80;                
           break;
           case SERIAL_FCR_RX_TRI4:
                data = data | 0xC0;                
           break;   
           default:
                data = data | 0x00;             
           break;           
            
           }    
    
    
    OUTW(port+SERIAL_FCR,data);

}	
//------------------------------------
void fLib_IRDA_Set_FCR_Trigl_Level_2(UINT32 port, UINT32 Tx_Tri_Level , UINT32 Rx_Tri_Level )//By 00/01/02/03
{
    UINT8 data;
    
    data=0x01; //FIFO Enable
    
    switch(Tx_Tri_Level)
          {
           case 0:
                data = data | 0x00;                
           break;
           case 1:
                data = data | 0x10;                
           break;    
           case 2:
                data = data | 0x20;                
           break;
           case 3:
                data = data | 0x30;                
           break;   
           default:
                data = data | 0x00;  
           break;           
            
           }
    
    switch(Rx_Tri_Level)
          {
           case 0:
                data = data | 0x00;                
           break;
           case 1:
                data = data | 0x40;                
           break;    
           case 2:
                data = data | 0x80;                
           break;
           case 3:
                data = data | 0xC0;                
           break;   
           default:
                data = data | 0x00;             
           break;           
            
           }    
    
    
    OUTW(port+SERIAL_FCR,data);

}	
//=========================================================================================
// Function Name: fLib_IRDA_Set_TST 
// Description: "TST=0x14"
//=========================================================================================				
void fLib_IRDA_Set_TST(UINT32 port,UINT8 setmode)
{
    OUTW(port + SERIAL_TST, setmode);
}


//=========================================================================================
// Function Name: fLib_IRDA_Set_MDR / fLib_IRDA_Enable_MDR / fLib_IRDA_Disable_MDR  =>ok1009
// Description: "MDR=0x20"
//=========================================================================================				
void fLib_IRDA_Set_MDR(UINT32 port,UINT8 setmode)
{
    OUTW(port + SERIAL_MDR, setmode);
}
//------------------------------------
void fLib_IRDA_Enable_MDR(UINT32 port,UINT8 setmode)
{
    UINT8 mdr;
    
    mdr=(UINT8)INW(port+SERIAL_MDR);
    OUTW(port + SERIAL_MDR, (mdr|setmode));

}
//------------------------------------
void fLib_IRDA_Disable_MDR(UINT32 port,UINT8 setmode)
{
    UINT8 mdr;
    
    mdr=(UINT8)INW(port+SERIAL_MDR);
    OUTW(port + SERIAL_MDR, (UINT32)(mdr&(~setmode)));

}

//=========================================================================================
// Function Name: fLib_IRDA_Disable_ACR / fLib_IRDA_Enable_ACR /fLib_IRDA_Disable_ACR
//                /fLib_IRDA_Set_ACR_STFIFO_Trigl_Level=>ok1009
// Description: Register "ACR=0x24"
//=========================================================================================	
void fLib_IRDA_Set_ACR(UINT32 port,UINT32 mode)
{
	//see IrDA spec P.42

	OUTW(port + SERIAL_ACR,  mode);
}
//------------------------------------
void fLib_IRDA_Enable_ACR(UINT32 port,UINT32 mode)
{
	//see IrDA spec P.42
	UINT32 data;

	data = INW(port + SERIAL_ACR);
	OUTW(port + SERIAL_ACR, data | mode);

}
//------------------------------------
void fLib_IRDA_Disable_ACR(UINT32 port,UINT32 mode)
{
	//see IrDA spec P.40
	UINT32 data;

	data = INW(port + SERIAL_ACR);
	OUTW(port + SERIAL_ACR,(data&(~mode)));

}								
//------------------------------------
void fLib_IRDA_Set_ACR_STFIFO_Trigl_Level(UINT32 port, UINT32 STFIFO_Tri_Level)
{
    UINT8 data;
    
   data = INW(port + SERIAL_ACR);

   data = data & SERIAL_ACR_STFF_CLEAR;

    switch(STFIFO_Tri_Level)
          {
           case SERIAL_ACR_STFF_TRGL_1:
                data = data | SERIAL_ACR_STFF_TRG1;                
           break;
           case SERIAL_ACR_STFF_TRGL_4:
                data = data | SERIAL_ACR_STFF_TRG2;                
           break;    
           case SERIAL_ACR_STFF_TRGL_7:
                data = data | SERIAL_ACR_STFF_TRG3;                
           break;
           case SERIAL_ACR_STFF_TRGL_8:
                data = data | SERIAL_ACR_STFF_TRG4;                
           break;   
           default:
                data = data | 0x00;  
           break;           
            
           }
    
    OUTW(port+SERIAL_ACR,data);

}	


//=========================================================================================
// Function Name: fLib_IRDA_Set_TXLEN_LSTFMLEN =>ok1009
// Description: "TXLENL=0x28" "TXLENH=0x2C" "LSTFMLENL=0x60" "LSTFMLENL=0x64"
//=========================================================================================				
void fLib_IRDA_Set_TXLEN_LSTFMLEN(UINT32 port,UINT32 Normal_FrmLength,UINT32 Last_FrmLength,UINT32 FrmNum)
{
	//see IrDA spec P.35 & P.44
	if(Normal_FrmLength>0x1fff)
	{
		Normal_FrmLength = 0x1fff;
	}
	else if(Normal_FrmLength == 0)
	{
		Normal_FrmLength = 0x1;	
	}	
    OUTW(port + SERIAL_TXLENH, ((Normal_FrmLength & 0x1f00) >> 8));
    OUTW(port + SERIAL_TXLENL, (Normal_FrmLength & 0xff));


    OUTW(port + SERIAL_LSTFMLENH, ((Last_FrmLength & 0x1f00) >> 8)+((FrmNum) << 5));
    OUTW(port + SERIAL_LSTFMLENL, (Last_FrmLength & 0xff));
}
//=========================================================================================
// Function Name: fLib_IRDA_Set_MaxLen =>ok1009
// Description: Register "MRXLENL=0x30" "MRXLENH=0x34"
//=========================================================================================
void fLib_IRDA_Set_MaxLen(UINT32 port,UINT32 Max_RxLength)
{
	//see IrDA spec P.36	( Max_RxLength = 1~27 )
	Max_RxLength = Max_RxLength + 5;

    OUTW(port + SERIAL_MRXLENH, ((Max_RxLength & 0x1f00) >> 8));
    OUTW(port + SERIAL_MRXLENL, (Max_RxLength & 0xff));
}
//=========================================================================================
// Function Name: fLib_IRDA_Set_PLR =>ok1009
// Description: "PLR=0x38"
//=========================================================================================	
void fLib_IRDA_Set_PLR(UINT32 port,UINT32 Preamble_Length)
{

	OUTW(port + SERIAL_PLR,  Preamble_Length);
}

//=========================================================================================
// Function Name: fLib_IRDA_Read_FMIIR =>ok1009
// Description: Register "FMIIR =0x3C"
//=========================================================================================
UINT8 fLib_IRDA_Read_FMIIR(UINT32 port)
{
	return ((UINT8)INW(port + SERIAL_FMIIR));
}


//=========================================================================================
// Function Name: fLib_SetFIRInt / fLib_IRDA_Enable_FMIIER / fLib_IRDA_Disable_FMIIER =>ok1009
// Description: Register "FMIIER=0x40"
//=========================================================================================
void fLib_IRDA_Set_FMIIER(UINT32 port,UINT32 mode)
{

	OUTW(port + SERIAL_FMIIER,  mode);

}
//------------------------------------
void fLib_IRDA_Enable_FMIIER(UINT32 port,UINT32 mode)
{

	UINT32 data;

	data = INW(port + SERIAL_FMIIER);
	OUTW(port + SERIAL_FMIIER, data | mode);
}									
//------------------------------------
void fLib_IRDA_Disable_FMIIER(UINT32 port,UINT32 mode)
{

	UINT32 data;
	data = INW(port + SERIAL_FMIIER);
	OUTW(port + SERIAL_FMIIER, (data &(~mode)));
}

//=========================================================================================
// Function Name: fLib_IRDA_Set_FMLSIER / fLib_IRDA_Enable_FMLSIER / fLib_IRDA_Disable_FMLSIER =>ok1009
// Description: Register "FMIIER=0x40"
//=========================================================================================
void fLib_IRDA_Set_FMLSIER(UINT32 port,UINT32 mode)
{
	OUTW(port + SERIAL_FMLSIER, mode);
}
//------------------------------------
void fLib_IRDA_Enable_FMLSIER(UINT32 port,UINT32 mode)
{
	//see IrDA spec P.42
	UINT32 data;

	data = INW(port + SERIAL_FMLSIER);
	OUTW(port + SERIAL_FMLSIER, data | mode);

}
//------------------------------------
void fLib_IRDA_Disable_FMLSIER(UINT32 port,UINT32 mode)
{
	//see IrDA spec P.40
	UINT32 data;
	data = INW(port + SERIAL_FMLSIER);
	OUTW(port + SERIAL_FMLSIER, (data&(~mode)));

}	

//=========================================================================================
// Function Name: fLib_IRDA_Read_STFF_STS =>ok1009
// Description: Register "STS=0x44"
//=========================================================================================
UINT8 fLib_IRDA_Read_STFF_STS(UINT32 port)
{
	return ((UINT8)INW(port + SERIAL_STFF_STS));
}							
//=========================================================================================
// Function Name: fLib_IRDA_Read_STFF_RXLEN =>ok1009
// Description: Register "STFF_RXLENL=0x48" "STFF_RXLENH=0x4C"
//=========================================================================================
UINT32 fLib_IRDA_Read_STFF_RXLEN(UINT32 port)
{
  UINT32 wRxFrameLength,wTemp1,wTemp2;
   
    
    wTemp1=INW(port+SERIAL_STFF_RXLENL);
    wTemp2=INW(port+SERIAL_STFF_RXLENH);
    wRxFrameLength=wTemp1|(wTemp2<<8);
    return ((UINT32)wRxFrameLength);
}
//=========================================================================================
// Function Name: fLib_IRDA_Read_RSR =>ok1009
// Description: Register "RSR=0x58"
//=========================================================================================
UINT8 fLib_IRDA_Read_RSR(UINT32 port)
{
	return ((UINT8)INW(port + SERIAL_RSR));
}

//=========================================================================================
// Function Name: fLib_IRDA_Read_RXFF_CNTR =>ok1009
// Description: Register "CNTR=0x5C"
//=========================================================================================
UINT8 fLib_IRDA_Read_RXFF_CNTR(UINT32 port)
{
	return ((UINT8)INW(port + SERIAL_RXFF_CNTR));
}

//=========================================================================================
// Function Name: fLib_IRDA_Read_FMLSR =>ok1009
// Description: Register "FMLSR=0x50"
//=========================================================================================
UINT8 fLib_IRDA_Read_FMLSR(UINT32 port)
{
	return ((UINT8)INW(port + SERIAL_FMLSR));
}


//=========================================================================================
// Function Name: fLib_IRDA_FIR_WRITE_CHAR
// Description: Register "SERIAL_THR=0x00"
//=========================================================================================
UINT32 fLib_IRDA_FIR_WRITE_CHAR(UINT32 port, char Ch)
{
  	UINT32 status; 
//    do
	{
	 	status=INW(port+SERIAL_FMIIR_PIO);
	}//while (((status & SERIAL_FMIIER_PIO_TT)!=SERIAL_FMIIER_PIO_TT));	
	// while TxFifo full...do nothing

    OUTW(port + SERIAL_THR,Ch);
    return status;
}


//########################################################################################################

/*
//=========================================================================================
// Function Name: fLib_IRDA_SIP_TIMER_ISR =>
// Description:
//=========================================================================================
void fLib_IRDA_SIP_TIMER_ISR(void)
{
  //100ms 
  fLib_ClearInt(FIQ_TIMER1);  
 
  if (IRDA_SIP_TimerCounter==5)
     {
     //Send SIP
     fLib_IRDA_Enable_ACR(IRDA_PORT_ForSIP,SERIAL_ACR_SEND_SIP);
     IRDA_SIP_TimerCounter=0;
     }
  else{
      //Count to 500 ms
      IRDA_SIP_TimerCounter++;
  
      }

}
//=========================================================================================
// Function Name: fLib_IRDA_SIP_TIMER_INIT =>
// Description:
//=========================================================================================
void fLib_IRDA_SIP_TIMER_INIT(UINT32 InitPort)
{
  //Init the  Timer:Timer1 ,100ms

   fLib_Timer_Init(1,10,fLib_IRDA_SIP_TIMER_ISR);
   IRDA_PORT_ForSIP=InitPort;


}


//=========================================================================================
// Function Name: fLib_IRDA_SIP_TIMER_DISABLE =>
// Description:
//=========================================================================================
void fLib_IRDA_SIP_TIMER_DISABLE(void)
{
 
fLib_IRDA_Disable_MDR(IRDA_PORT_ForSIP,SERIAL_MDR_SIP_BY_CPU);
 fLib_Timer_Close(1);

}
*/
//=======================================================================================
// Function Name: fLib_SetIRTxInv
// Description:
//======================================================================================
void fLib_SetIRTxInv(UINT32 port,UINT32 enable)
{	
	UINT32 mdr;
	
    mdr = INW(port + SERIAL_MDR);
    if(enable == 1)
    {
    	mdr=mdr | SERIAL_MDR_IITx;
    }
    else
    {
     	mdr=mdr & (~SERIAL_MDR_IITx);   
    }	
    OUTW(port + SERIAL_MDR, mdr);


}

//=======================================================================================
// Function Name: fLib_SetIRRxInv
// Description:
//======================================================================================
void fLib_SetIRRxInv(UINT32 port,UINT32 enable)
{	
	UINT32 mdr;
	
    mdr = INW(port + SERIAL_MDR);
    if(enable == 1)
    {
    	mdr=mdr | SERIAL_MDR_FIRx;
    }
    else
    {
     	mdr=mdr & (~SERIAL_MDR_FIRx);   
    }	
    OUTW(port + SERIAL_MDR, mdr);


}
//************************************* Auto Mode Select *********************************
//=======================================================================================
// Function Name: fLib_IrDA_DoDelay
// Description:
//======================================================================================
UINT8 fLib_IrDA_DoDelay(void)
{	
 UINT8  bDumy,i; 
    bDumy=0;
    for (i=1;i<100;i++)
        bDumy++;
        
return bDumy;

}





//=======================================================================================
// Function Name: fLib_IrDA_AutoMode_SIR_Low
// Description:
//======================================================================================
void fLib_IrDA_AutoMode_SIR_Low(UINT32 port)
{	
  UINT32 mdr;
	

	mdr = INW(port + SERIAL_MCR);
	mdr|=(SERIAL_MCR_OUT3);
	OUTW(port + SERIAL_MCR,mdr); //Poll low ModeSelect
	


}

//=======================================================================================
// Function Name: fLib_IrDA_AutoMode_FIR_High
// Description:
//======================================================================================
void fLib_IrDA_AutoMode_FIR_High(UINT32 port)
{	
  UINT32 mdr;
	

	mdr = INW(port + SERIAL_MCR);
	mdr&=(~SERIAL_MCR_OUT3);
	OUTW(port + SERIAL_MCR,mdr); //Poll Hight ModeSelect
	


}


//=================== SIR PIO Function Call==================================================
//** Lib function code
// 1.TX Init => fLib_SIR_TX_Init()
// 2.RX Init => fLib_SIR_RX_Init()
// 3.Send Data => fLib_SIR_TX_Data()
// 4.Receive Data => fLib_SIR_RX_ISR();
// 5.SIR TX RX Disable => fLib_SIR_TXRX_Close(); //disable TXRX



//=======================================================================================
// Function Name: fLib_SIR_RX_ISR
// Description:The ISR of receive data
//======================================================================================
/*void fLib_SIR_RX_ISR(void)
{
	UINT32 status;		
		
	fLib_ClearInt(SIR_RX_Device.IRQ_Num);
 	status=INW(SIR_RX_Device.port+SERIAL_LSR);
	   
	while ((status & SERIAL_LSR_DR)==SERIAL_LSR_DR)	// wait until Rx ready
	{
    	*(SIR_RX_Device.cptRXDataBuffer+SIR_RX_Device.Length) = INW(SIR_RX_Device.port + SERIAL_RBR);
    	SIR_RX_Device.Length++;    
 		status=INW(SIR_RX_Device.port+SERIAL_LSR);
	} 

}*/

//************************************ SIR DMA Function Call **********************************
//** Lib function code
// 1.TX Init => fLib_SIR_TX_Init_DMA()
// 2.RX Init => fLib_SIR_RX_Init_DMA()
// 3.Send Data => fLib_SIR_TX_Data_DMA()
// 4.SIR TX RX Disable => fLib_SIR_TXRX_Close_DMA(); //disable TXRX
/*
//=======================================================================================
// Function Name: fLib_IRDA_Enable_DMA2
// Description: 
// 
//======================================================================================
void fLib_IRDA_Enable_DMA2(UINT32 port)
{
	//see IrDA spec P.42
	UINT32 data;

	data = INW(port + SERIAL_MCR);
	OUTW(port + SERIAL_MCR, data | SERIAL_MCR_DMA2);

}
//=======================================================================================
// Function Name: fLib_IRDA_Disable_DMA2
// Description: 
// 
//======================================================================================
void fLib_IRDA_Disable_DMA2(UINT32 port)
{
	//see IrDA spec P.42
	UINT32 data;

	data = INW(port + SERIAL_MCR);
	OUTW(port + SERIAL_MCR, data & (~SERIAL_MCR_DMA2));

}

//=======================================================================================
// Function Name: fLib_SIR_TX_Init_DMA
// Description: 
// 
//======================================================================================
void fLib_SIR_TX_Init_DMA(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwTX_Tri_Value,UINT32 bInv)
{
UINT32 dwValue,PSR_Value;

SIR_TX_Device.port=port;
  //1.Set DLAB=1
    OUTW(port + SERIAL_LCR, SERIAL_LCR_DLAB);	

  //2.Set PSR
	PSR_Value = (UINT32)(IRDA_CLK/PSR_CLK);
	if(PSR_Value>0x1f)
	{
		PSR_Value = 0x1f;
	}	
	OUTW(port + SERIAL_PSR, PSR_Value);
	      
      
  //3.Set DLM/DLL  => Set baud rate 
    OUTW(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
    OUTW(port + SERIAL_DLL, (baudrate & 0xff));
 
  //4.Set DLAB=0
    OUTW(port + SERIAL_LCR,0x00);	
  
  //5.LCR Set Character=8 / Stop_Bit=1 / Parity=Disable
    OUTW(port + SERIAL_LCR,0x03);	
  		
  //6.Set MDR->SIR  
    fLib_SetSerialMode(port, SERIAL_MDR_SIR);

  //7.Set TX INV 
    fLib_SetIRTxInv(port,bInv);
 
  //8.Reset TX FIFO & 
    fLib_IRDA_Enable_FCR(port,SERIAL_FCR_TX_FIFO_RESET);      //Reset TX FIFO

 
  //9.Enable FIFO and Set Tri level of TX
    //fLib_IRDA_Set_FCR(port,SERIAL_FCR_TXRX_FIFO_ENABLE);      //Enable FIFO          => Enable in the last step
    fLib_IRDA_Set_FCR_Trigl_Level_2(port,dwTX_Tri_Value,0);//Set RX Trigger Level  
  
  
  //10.Enable IER(...)
   	fLib_SetSerialInt(port,0x00);


  //11.Set ACR 1.6/316 & Enable the TX
    dwValue=0;
    dwValue|=SIP_PW_Value;        //Set SIP PW 
    dwValue|=SERIAL_ACR_TXENABLE; //Enable TX
    OUTW(port + SERIAL_ACR, dwValue);


}
//=======================================================================================
// Function Name: fLib_SIR_RX_Init_DMA
// Description: 
// 
//======================================================================================
void fLib_SIR_RX_Init_DMA(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwRX_Tri_Value,UINT32 ReceiveDataAddr,UINT32 bInv,UINT32 dwReceiveDataLength)
{

UINT32 dwValue,PSR_Value;
UINT32 dwAPB_DMA_RX_Port,dwAPB_DMA_Reg_Port;


SIR_RX_Device.port=port;
  //1.Set DLAB=1
    OUTW(port + SERIAL_LCR, SERIAL_LCR_DLAB);	

  //2.Set PSR
	PSR_Value = (UINT32)(IRDA_CLK/PSR_CLK);
	if(PSR_Value>0x1f)
	{
		PSR_Value = 0x1f;
	}	
	OUTW(port + SERIAL_PSR, PSR_Value);
	      
      
  //3.Set DLM/DLL  => Set baud rate 
    OUTW(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
    OUTW(port + SERIAL_DLL, (baudrate & 0xff));
 
  //4.Set DLAB=0
    OUTW(port + SERIAL_LCR,0x00);	
  
  //5.LCR Set Character=8 / Stop_Bit=1 / Parity=Disable
    OUTW(port + SERIAL_LCR,0x03);	
  		
  //5.Set MDR->SIR  
    fLib_SetSerialMode(port, SERIAL_MDR_SIR);

  //6.Set TX INV 
    fLib_SetIRRxInv(port,bInv); 

  //7.Reset RX FIFO & 
    fLib_IRDA_Enable_FCR(port,SERIAL_FCR_RX_FIFO_RESET);      //Reset RX FIFO

 
  //8.Enable FIFO and Set Tri level of RX
    fLib_IRDA_Set_FCR(port,SERIAL_FCR_TXRX_FIFO_ENABLE);      //Enable FIFO
    fLib_IRDA_Set_FCR_Trigl_Level_2(port,0,dwRX_Tri_Value);//Set RX Trigger Level  
  
  //9.Clear the RX Data Buffer 
    SIR_RX_Device.Length=0;
    SIR_RX_Device.cptRXDataBuffer=ReceiveDataAddr;
  
  //10.Enable IER(...)
    fLib_SetSerialInt(port,SERIAL_IER_DR);
  
    if (port==CPE_UART1_BASE)//port1
       SIR_RX_Device.IRQ_Num=FIQ_UART1;
    else //port2   
       SIR_RX_Device.IRQ_Num=IRQ_UART2;
       
    fLib_IRDA_INT_Init(SIR_RX_Device.IRQ_Num,fLib_SIR_RX_ISR);

#ifdef IRDA_SIR_USEDMA_OLD
  //11.Init the DMA
      if (port==0x98200000)
        {
        dwAPB_DMA_RX_Port=AHB2APB_DMA_REQ_IR1_RX;
        dwAPB_DMA_Reg_Port=AHB2APB_DMA_A;
        };
    if (port==0x98300000)
        {
         dwAPB_DMA_RX_Port=AHB2APB_DMA_REQ_IR2_RX;	
         dwAPB_DMA_Reg_Port=AHB2APB_DMA_B;
        };	


#else


  //11.Init the DMA
      if (port==0x98200000)
        {
        dwAPB_DMA_RX_Port=AHB2APB_DMA_REQ_IR1_RX;
        //dwAPB_DMA_Reg_Port=AHB2APB_DMA_C;    //06142003;;Bruce;;use DMA-C or DMA-D 
        dwAPB_DMA_Reg_Port=AHB2APB_DMA_A;    //06142003;;Bruce;;use DMA-C or DMA-D 

        };
    if (port==0x98300000)
        {
         dwAPB_DMA_RX_Port=AHB2APB_DMA_REQ_IR2_RX;	
         //dwAPB_DMA_Reg_Port=AHB2APB_DMA_D;//06142003;;Bruce;;use DMA-C or DMA-D 
         dwAPB_DMA_Reg_Port=AHB2APB_DMA_B;//06142003;;Bruce;;use DMA-C or DMA-D 

        };	

#endif





        
      fLib_APBDMA_Init(dwAPB_DMA_Reg_Port, (port + SERIAL_RBR), ReceiveDataAddr//port,Source,Target,Total_bytes, Data Format ,Burst
                        ,dwReceiveDataLength,AHB2APB_DMA_BYTE, FALSE);
      
      fLib_APBDMA_Config( dwAPB_DMA_Reg_Port, dwAPB_DMA_RX_Port,AHB2APB_DMA_SRC_FIX
                         ,AHB2APB_DMA_DES_INC1, AHB2APB_DMA_SRC_APB, AHB2APB_DMA_DES_AHB);
      
      //fLib_APBDMA_Interrupt( SelectPort.DMA_Reg_ADD, TRUE, TRUE);
      
      fLib_APBDMA_EnableTrans( dwAPB_DMA_Reg_Port, TRUE);

#ifdef IRDA_ENABLE_CACHE      
     CPUCleanInvalidateDCacheAll();
#endif

  //Enable IrDA SIR DMA
    fLib_IRDA_Enable_DMA2(port);

  //12.Set ACR 1.6/316 & Enable the RX
    dwValue=0;
    dwValue|=SERIAL_ACR_RXENABLE; //Enable RX
    OUTW(port + SERIAL_ACR, dwValue);

}
//=======================================================================================
// Function Name: fLib_SIR_TX_Data_DMA
// Description:Transmit the data
// Input:  1.Data buffer
//         2.Data length
// Output: void 
//======================================================================================
void fLib_SIR_TX_Data_DMA(UINT32 dwport,char *bpData, UINT32 dwLength)
{

UINT32 dwAPB_DMA_TX_Port,dwAPB_DMA_Reg_Port;
  //1.Set APB DMA ...
    if (dwport==0x98200000)
        {
        dwAPB_DMA_TX_Port=AHB2APB_DMA_REQ_IR1_TX;
        //dwAPB_DMA_Reg_Port=AHB2APB_DMA_A;
        dwAPB_DMA_Reg_Port=AHB2APB_DMA_C;
        
        }
    else
        {
         dwAPB_DMA_TX_Port=AHB2APB_DMA_REQ_IR2_TX;	
         //dwAPB_DMA_Reg_Port=AHB2APB_DMA_B;
         dwAPB_DMA_Reg_Port=AHB2APB_DMA_D;
         
        }	
  
    fLib_APBDMA_Init( dwAPB_DMA_Reg_Port, bpData,(dwport + SERIAL_THR) //port,Source,Target
                     , dwLength , AHB2APB_DMA_BYTE, FALSE); //Total bytes, Data Format ,Burst
    fLib_APBDMA_Config( dwAPB_DMA_Reg_Port, dwAPB_DMA_TX_Port, AHB2APB_DMA_SRC_INC1
                      , AHB2APB_DMA_DES_FIX, AHB2APB_DMA_SRC_AHB, AHB2APB_DMA_DES_APB);
    //fLib_APBDMA_Interrupt( SelectPort.DMA_Reg_ADD, TRUE, TRUE);
    fLib_APBDMA_EnableTrans( dwAPB_DMA_Reg_Port, TRUE);
#ifdef IRDA_ENABLE_CACHE      
    CPUCleanInvalidateDCacheAll();
#endif
 
 
  
  //2.Enable IrDA SIR DMA
   fLib_IRDA_Enable_DMA2(dwport); 
  //3.Enable FIFO 
   fLib_IRDA_Set_FCR(dwport,SERIAL_FCR_TXRX_FIFO_ENABLE);      //Enable FIFO          => Enable in the last step 







}

//=======================================================================================
// Function Name: fLib_SIR_TXRX_Close_DMA
// Description:
//======================================================================================
void fLib_SIR_TXRX_Close_DMA(void)
{

//1.Disable the TX RX
  fLib_IRDA_Disable_ACR(SIR_RX_Device.port,SERIAL_ACR_TX_ENABLE);
  fLib_IRDA_Disable_ACR(SIR_RX_Device.port,SERIAL_ACR_RX_ENABLE);  

  fLib_IRDA_Disable_ACR(SIR_TX_Device.port,SERIAL_ACR_TX_ENABLE); 
  fLib_IRDA_Disable_ACR(SIR_TX_Device.port,SERIAL_ACR_RX_ENABLE);

//2.Disable the FIFO
  fLib_IRDA_Set_FCR(SIR_RX_Device.port,SERIAL_FCR_FE);     
  fLib_IRDA_Set_FCR(SIR_TX_Device.port,SERIAL_FCR_FE);

//3.Disable DMA
  fLib_IRDA_Disable_DMA2(SIR_RX_Device.port);
  fLib_IRDA_Disable_DMA2(SIR_TX_Device.port);  

//4.Disable the APB DMA Enable  
  fLib_APBDMA_EnableTrans( AHB2APB_DMA_A, FALSE); 
  fLib_APBDMA_EnableTrans( AHB2APB_DMA_B, FALSE); 
}	



*/
//=======================================================================================
// Function Name: fLib_SIR_TX_Init
// Description: 
// 
//======================================================================================
// baudrate : IRDA_BAUD_115200(1) / IRDA_BAUD_57600(2) / IRDA_BAUD_38400 /...
// port		: should be virtual address
//
/*
void fLib_SIR_TX_Init(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwTX_Tri_Value,UINT32 bInv)
{
UINT32 dwValue,PSR_Value;

SIR_TX_Device.port=port;


//........ change speed
  //1.Set DLAB=1
    OUTW(port + SERIAL_LCR, SERIAL_LCR_DLAB);	

  //2.Set PSR
	PSR_Value = (UINT32)(IRDA_CLK/PSR_CLK);
	if(PSR_Value>0x1f)
	{
		PSR_Value = 0x1f;
	}	
	OUTW(port + SERIAL_PSR, PSR_Value);
	      
      
  //3.Set DLM/DLL  => Set baud rate 
    OUTW(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
    OUTW(port + SERIAL_DLL, (baudrate & 0xff));
 
  //4.Set DLAB=0
    OUTW(port + SERIAL_LCR,0x00);	

//........ set
  //5.LCR Set Character=8 / Stop_Bit=1 / Parity=Disable
    OUTW(port + SERIAL_LCR,0x03);	
  		
  //6.Set MDR->SIR  
    fLib_SetSerialMode(port, SERIAL_MDR_SIR);

  //7.Set TX INV 
    fLib_SetIRTxInv(port,bInv);
 
  //8.Reset TX FIFO & 
    fLib_IRDA_Enable_FCR(port,SERIAL_FCR_TX_FIFO_RESET);      //Reset TX FIFO

 
  //9.Enable FIFO and Set Tri level of TX
    fLib_IRDA_Set_FCR(port,SERIAL_FCR_TXRX_FIFO_ENABLE);      //Enable FIFO
    fLib_IRDA_Set_FCR_Trigl_Level_2(port,dwTX_Tri_Value,0);//Set RX Trigger Level  
  
  
  //10.Enable IER(...)
   	fLib_SetSerialInt(port,0x00);


  //11.Set ACR 1.6/316 & Enable the TX
    dwValue=0;
    dwValue|=SIP_PW_Value;        //Set SIP PW 
    dwValue|=SERIAL_ACR_TXENABLE; //Enable TX
    OUTW(port + SERIAL_ACR, dwValue);

}
//=======================================================================================
// Function Name: fLib_SIR_RX_Init
// Description: 
// 
//======================================================================================

void fLib_SIR_RX_Init(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwRX_Tri_Value,UINT32 ReceiveDataAddr,UINT32 bInv)

{

UINT32 dwValue,PSR_Value;

SIR_RX_Device.port=port;
  //1.Set DLAB=1
    OUTW(port + SERIAL_LCR, SERIAL_LCR_DLAB);	

  //2.Set PSR
	PSR_Value = (UINT32)(IRDA_CLK/PSR_CLK);
	if(PSR_Value>0x1f)
	{
		PSR_Value = 0x1f;
	}	
	OUTW(port + SERIAL_PSR, PSR_Value);
	      
      
  //3.Set DLM/DLL  => Set baud rate 
    OUTW(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
    OUTW(port + SERIAL_DLL, (baudrate & 0xff));
 
  //4.Set DLAB=0
    OUTW(port + SERIAL_LCR,0x00);	
  
  //5.LCR Set Character=8 / Stop_Bit=1 / Parity=Disable
    OUTW(port + SERIAL_LCR,0x03);	
  		
  //5.Set MDR->SIR  
    fLib_SetSerialMode(port, SERIAL_MDR_SIR);

  //6.Set TX INV 
    fLib_SetIRRxInv(port,bInv); 

  //7.Reset RX FIFO & 
    fLib_IRDA_Enable_FCR(port,SERIAL_FCR_RX_FIFO_RESET);      //Reset RX FIFO

 
  //8.Enable FIFO and Set Tri level of RX
    fLib_IRDA_Set_FCR(port,SERIAL_FCR_TXRX_FIFO_ENABLE);      //Enable FIFO
    fLib_IRDA_Set_FCR_Trigl_Level_2(port,0,dwRX_Tri_Value);//Set RX Trigger Level  
  
  //9.Clear the RX Data Buffer 
    SIR_RX_Device.Length=0;
    SIR_RX_Device.cptRXDataBuffer=ReceiveDataAddr;
  
  //10.Enable IER(...)
    fLib_SetSerialInt(port,SERIAL_IER_DR);
  
    if (port==CPE_UART1_BASE)//port1
       SIR_RX_Device.IRQ_Num=FIQ_UART1;
    else //port2   
       SIR_RX_Device.IRQ_Num=IRQ_UART2;
       
    fLib_IRDA_INT_Init(SIR_RX_Device.IRQ_Num,fLib_SIR_RX_ISR);


  //11.Set ACR 1.6/316 & Enable the RX
    dwValue=0;
    dwValue|=SERIAL_ACR_RXENABLE; //Enable RX
    OUTW(port + SERIAL_ACR, dwValue);

}*/
//=======================================================================================
// Function Name: fLib_SIR_TX_Data
// Description:Transmit the data
// Input:  1.Data buffer
//         2.Data length
// Output: void 
//======================================================================================
void fLib_SIR_TX_Data(UINT32 dwport,char *bpData, UINT32 dwLength)
{
 UINT32 i;

   	for(i=0; i<dwLength; i++)
   	{
		fLib_PutSerialChar(dwport,*(bpData+i));	    			  
   	};	



}

//=======================================================================================
// Function Name: fLib_SIR_TXRX_Close
// Description:
//======================================================================================
void fLib_SIR_TXRX_Close(void)
{
//Makesure the TX FIFO is empty


//Disable the TX RX
  fLib_IRDA_Disable_ACR(SIR_RX_Device.port,SERIAL_ACR_TX_ENABLE);
  fLib_IRDA_Disable_ACR(SIR_RX_Device.port,SERIAL_ACR_RX_ENABLE);  

  fLib_IRDA_Disable_ACR(SIR_TX_Device.port,SERIAL_ACR_TX_ENABLE); 
  fLib_IRDA_Disable_ACR(SIR_TX_Device.port,SERIAL_ACR_RX_ENABLE);

//2.Reset TX/RX FIFO 
  fLib_IRDA_Enable_FCR(SIR_RX_Device.port,SERIAL_FCR_RX_FIFO_RESET);      //Reset RX FIFO
  fLib_IRDA_Enable_FCR(SIR_RX_Device.port,SERIAL_FCR_TX_FIFO_RESET);      //Reset TX FIFO 	

  fLib_IRDA_Enable_FCR(SIR_TX_Device.port,SERIAL_FCR_RX_FIFO_RESET);      //Reset RX FIFO
  fLib_IRDA_Enable_FCR(SIR_TX_Device.port,SERIAL_FCR_TX_FIFO_RESET);      //Reset TX FIFO 		
  
//3.Disable the FIFO
  fLib_IRDA_Set_FCR(SIR_RX_Device.port,SERIAL_FCR_FE);     
  fLib_IRDA_Set_FCR(SIR_TX_Device.port,SERIAL_FCR_FE);
}	

//=======================================================================================
// Function Name: fLib_SIR_TXRX_Close
// Description:
//======================================================================================
void fLib_SIR_TX_Close(void)
{
  UINT32 status;
  
  //Waiting data TX finish
         //Makesure the TX FIFO is empty
  do{
	 	status=INW(SIR_TX_Device.port+SERIAL_LSR);
	}while (!((status & SERIAL_LSR_THRE)==SERIAL_LSR_THRE));	// wait until Tx ready                  


//Disable the TX RX
  fLib_IRDA_Disable_ACR(SIR_TX_Device.port,SERIAL_ACR_TX_ENABLE); 
  fLib_IRDA_Disable_ACR(SIR_TX_Device.port,SERIAL_ACR_RX_ENABLE);

//2.Reset TX/RX FIFO 
  fLib_IRDA_Enable_FCR(SIR_TX_Device.port,SERIAL_FCR_RX_FIFO_RESET);      //Reset RX FIFO
  fLib_IRDA_Enable_FCR(SIR_TX_Device.port,SERIAL_FCR_TX_FIFO_RESET);      //Reset TX FIFO 		
  
//3.Disable the FIFO
  fLib_IRDA_Set_FCR(SIR_TX_Device.port,SERIAL_FCR_FE);
}	
//=======================================================================================
// Function Name: fLib_SIR_TXRX_Close
// Description:
//======================================================================================
void fLib_SIR_RX_Close(void)
{

//Disable the TX RX
  fLib_IRDA_Disable_ACR(SIR_RX_Device.port,SERIAL_ACR_TX_ENABLE);
  fLib_IRDA_Disable_ACR(SIR_RX_Device.port,SERIAL_ACR_RX_ENABLE);  

//2.Reset TX/RX FIFO 
  fLib_IRDA_Enable_FCR(SIR_RX_Device.port,SERIAL_FCR_RX_FIFO_RESET);      //Reset RX FIFO
  fLib_IRDA_Enable_FCR(SIR_RX_Device.port,SERIAL_FCR_TX_FIFO_RESET);      //Reset TX FIFO 	
  
//3.Disable the FIFO
  fLib_IRDA_Set_FCR(SIR_RX_Device.port,SERIAL_FCR_FE);     
}	

//------------------paulong
//	speed=9600,19200..115200	SIR
int fLib_SetSIRSpeed(u32 port,__u32 speed)	
{
	UINT32 baudrate,PSR_Value;
//........ change speed
  //1.Set DLAB=1
    OUTW(port + SERIAL_LCR, SERIAL_LCR_DLAB);	

  //2.Set PSR
	PSR_Value = (UINT32)(IRDA_CLK/PSR_CLK);
	if(PSR_Value>0x1f)
	{
		PSR_Value = 0x1f;
	}	
	OUTW(port + SERIAL_PSR, PSR_Value);
	
	 //3.Set DLM/DLL  => Set baud rate      
	switch (speed) {
		case 9600:  
				baudrate=IRDA_BAUD_9600; 
				break;
		case 19200: 
				baudrate=IRDA_BAUD_19200;
				break;
		case 38400: 
				baudrate=IRDA_BAUD_38400;
				break;
		case 57600: 
				baudrate=IRDA_BAUD_57600;
				break;
		case 115200:
				baudrate=IRDA_BAUD_115200;
				break;
		default:
				printk("fLib_SetSpeed(%d) unknow speed\n",speed);
				return -1;				
	}
  	
    OUTW(port + SERIAL_DLM, ((baudrate & 0xf00) >> 8));
    OUTW(port + SERIAL_DLL, (baudrate & 0xff));
 
  //4.Set DLAB=0
    OUTW(port + SERIAL_LCR,0x00);	
          	
   	return 0;
}
//---------------------------------
// mode=SERIAL_MDR_FIR/SERIAL_MDR_SIR/SERIAL_MDR_UART
void fLib_SetIRMode(u32 port,u32 mode)
{
	switch(mode)
	{
	case SERIAL_MDR_FIR:	
						fLib_IrDA_AutoMode_FIR_High(port);					
						fLib_SetSerialMode(port,mode);
						
						break;
	case SERIAL_MDR_SIR:
						fLib_IrDA_AutoMode_SIR_Low(port);
						OUTW(port+SERIAL_LCR,SERIAL_LCR_LEN8);	//8N1
						fLib_SetIRTxInv(port,0);	//test test paulong not inverse
						fLib_SetIRRxInv(port,0);	//test test paulong	not inverse		
						fLib_SetSerialMode(port,mode);
												
						break;
	}				
	
}
//-------------------------------
void fLib_SetFIFO(u32 port)
{	
	// reset TX /RX
	fLib_IRDA_Enable_FCR(port,	SERIAL_FCR_TX_FIFO_RESET);
	fLib_IRDA_Enable_FCR(port,	SERIAL_FCR_TX_FIFO_RESET);
	fLib_IRDA_Enable_FCR(port,	SERIAL_FCR_RX_FIFO_RESET);
	fLib_IRDA_Enable_FCR(port,	SERIAL_FCR_RX_FIFO_RESET);
	
	// set FIFO Triggle level
	fLib_IRDA_Set_FCR_Trigl_Level_2(port,TX_TRIG_LEVEL,RX_TRIG_LEVEL);
	
	// enable FIFO
	fLib_IRDA_Set_FCR(port,		SERIAL_FCR_TXRX_FIFO_ENABLE);
	
}
//------------------------------
int fLib_irda_probe(u32 iobase)
{
	u8 test;
	P_DEBUG( __FUNCTION__ "() 1\n");
	
	P_DEBUG( __FUNCTION__ "() iobase=0x%lX IO_ADDRESS1(iobase)=0x%lX\n",iobase,IO_ADDRESS1(iobase));	
	test=INB(iobase+SERIAL_FMLSR);
	P_DEBUG( __FUNCTION__ "() 2\n");
	if(test==0xc3)
		{
			printf("Faraday IRDA driver loaded. \n"	);
			return 0;
		}
	else
		return -1;

}
//----------------------------------------
void DumpBuff(char *buff,u32 count)
{
	u32 i;
	for(i=0;i<count;i++)
		printk("0x%X ",buff[i]);
	printk("\n");	
}
//------------------------------------------
void DumpRegister(u32 Base,u32 Last)
{
	u32 i;
	printk("DumpRegister(0x%X,0x%X)\n",Base,Last);
	for(i=Base;i<Last;i+=4)
		printk("0x%X ",INW(i));
	printk("\n");	
}
//---------------------------------------------
void Sir_Enable_Tx(u32 port)		
{	
	fLib_IRDA_Disable_ACR(port,SERIAL_ACR_RXENABLE);
	fLib_IRDA_Enable_ACR(port,SERIAL_ACR_TXENABLE);
	
	fLib_SetSerialInt(port,SERIAL_IER_TE|SERIAL_IER_RLS);
}
//-----------------------------------------
void Sir_Enable_Rx(u32 port)					 
{
	fLib_IRDA_Disable_ACR(port,SERIAL_ACR_TXENABLE);
	fLib_IRDA_Enable_ACR(port,SERIAL_ACR_RXENABLE);
	fLib_SetSerialInt(port, SERIAL_IER_DR|SERIAL_IER_RLS);
}
//-----------------------------------------------------
void  Sir_Disable_TxRx(u32 port)  
{
	fLib_IRDA_Disable_ACR(port,SERIAL_ACR_RXENABLE);
	fLib_IRDA_Disable_ACR(port,SERIAL_ACR_TXENABLE);
	fLib_ResetSerialInt(port);
	
}
