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
* Name: irda_SIR_Test.c                                                    *
* Description: IRDA test program                                           *
* Author: Andrew                                                           *
* Date: 2002/10/01                                                         *
* Version:1.0                                                              *
*--------------------------------------------------------------------------*
****************************************************************************/
//#include <stdlib.h>
//#include <stdio.h>

//#include "fLib.h"
//#include "cpe_serial.h"   
#include "irda.h"
//#include "irda_Test.h"
//#include "APB_DMA.h"

/*
char IRDA_Test_Data[256]=
     {
      0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
      0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
      0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
      0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,
      0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
      0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
      0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
      0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
      0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,
      0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,
      0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
      0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,0xBB,
      0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,
      0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,
      0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  
   };      

IRDA_SIT_TEST_PATERN_STRUCTURE       SIR_Test_Patern;

*/

//************************************* SIR Test ****************************************
//** Demo code
// 1.TXRX
//=======================================================================================
// Function Name: SIR_Test_Patern_Init
// Description: a= BAUD Rate
//              b= SIP
//              c= Tri Level
//              d= TX Pulse Inverse
//======================================================================================
void SIR_Test_Patern_Init(void)
{
	
 UINT32 dwGet_Size;
 int i;
 	
 //1.Set BaudRate
    SIR_Test_Patern.BaudRate[0]=IRDA_BAUD_115200;//1 
    SIR_Test_Patern.BaudRate[1]=IRDA_BAUD_57600; //2 
    SIR_Test_Patern.BaudRate[2]=IRDA_BAUD_38400; //3 
    SIR_Test_Patern.BaudRate[3]=IRDA_BAUD_19200; //6 
    SIR_Test_Patern.BaudRate[4]=IRDA_BAUD_14400; //8 
    SIR_Test_Patern.BaudRate[5]=IRDA_BAUD_9600;  //12
    
//2.Set SIP
    SIR_Test_Patern.SIP[0]=0;//0
    SIR_Test_Patern.SIP[1]=1;//1
    
//3.Set SIP
    SIR_Test_Patern.Tri_Level[0]=0;//0
    SIR_Test_Patern.Tri_Level[1]=1;//1
    SIR_Test_Patern.Tri_Level[2]=2;//2
    SIR_Test_Patern.Tri_Level[3]=3;//3
//4.Set SIP    
    SIR_Test_Patern.TX_Pules_Inverse[0]=0;//0
    SIR_Test_Patern.TX_Pules_Inverse[1]=1;//1


//5.prepare Test Data
    SIR_TX_Device.cptTXDataBuffer=SIR_TEST_TX_DATA_BASE_ADDR;

    dwGet_Size=0;
    
      for (i=0;i<SIR_TEST_TXRX_SIZE;i++) 
           {memcpy((SIR_TX_Device.cptTXDataBuffer+dwGet_Size),IRDA_Test_Data,256);
           dwGet_Size+=256;
           }
         
     SIR_TX_Device.Length=dwGet_Size;


}


/*
//=======================================================================================
// Function Name: SIR_Test_RreeRun_PIO_2
// Description: a= BAUD Rate
//              b= SIP
//              c= Tri Level
//              d= TX Pulse Inverse
//======================================================================================
void SIR_Test_RreeRun_PIO_2(void)
{

char *cptTx,*cptRx;
int i,j;
UINT8 a,b,c,d,bCompareResult,bFreeRun,dwFailCounter,bRole,bPortSelect;
UINT32 dwFreeRunCounter,dwPortAddress;


   printf("Role Select <1>.TX First <2>.RX First:");
   scanf("%d",&bRole);
   printf("\n");

   printf("Port Select <1>.IrDA-1 <2>.IrDA-2:");
   scanf("%d",&bPortSelect);
   printf("\n");

   printf("Free Run(1/0->Y/N): ");
   scanf("%d",&bFreeRun);
   printf("\n");


   
   if (bPortSelect==1)
       dwPortAddress=CPE_UART1_BASE;
   else 
       dwPortAddress=CPE_UART2_BASE; 
   
   
SIR_Test_Patern_Init();



dwFailCounter=0;
dwFreeRunCounter=0;
do{

for (a=0;a<6;a++) 
    {
    if (bRole==1)
        {switch (a)
               {
            	case 0:
           	     printf("*** SIR-PIO, Size = %d ,BR = 115200 Testing...\n",SIR_TX_Device.Length);
            	break;
           	    case 1:
           	         printf("*** SIR-PIO, Size = %d ,BR = 57600 Testing...\n",SIR_TX_Device.Length);
           	    break;
           	    case 2:
           	         printf("*** SIR-PIO, Size = %d ,BR = 38400 Testing...\n",SIR_TX_Device.Length);
           	    break;
           	    case 3:
           	         printf("*** SIR-PIO, Size = %d ,BR = 19200 Testing...\n",SIR_TX_Device.Length);
           	    break;
           	    case 4:
           	         printf("*** SIR-PIO, Size = %d ,BR = 14400 Testing...\n",SIR_TX_Device.Length);
           	    break;
           	    case 5:
           	         printf("*** SIR-PIO, Size = %d ,BR = 9600 Testing...\n",SIR_TX_Device.Length);
           	    break;
        
               	}
        }


    for (b=0;b<2;b++)	
    	{
    	 for(c=0;c<4;c++)
    	    {
    	     for (d=0;d<1;d++)
    	         {
                 
                 for (j=0;j<2;j++)//Role change
                     {
    	              dwFreeRunCounter++;             
                      if (bRole==1) 
                        {//-------------- IrDA TX Data ------------------------------
                   
    	                 //<1>.Init TX =>Select Port    
                               fLib_SIR_TX_Init(dwPortAddress, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_Test_Patern.TX_Pules_Inverse[d]);
                         
                         //<2>.Send Data 
                               fLib_SIR_TX_Data(SIR_TX_Device.port,SIR_TX_Device.cptTXDataBuffer, SIR_TX_Device.Length);
                         
 
                         //<3>.Close device
                               fLib_SIR_TX_Close();
                   
                         bRole=2;
                   
                    }else{//-------------- IrDA RX Data ------------------------------
                          //<1>.Init RX =>Select Port   
                                fLib_SIR_RX_Init(dwPortAddress, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_TEST_RX_DATA_BASE_ADDR,0);
                          
                          //<4>.Receive data
                               do{
                           
                                 }while(SIR_RX_Device.Length<SIR_TX_Device.Length);               
                         
                          //<3>.Close device
                                fLib_SIR_RX_Close();                
                         
                         //<6>.Compare data 
                               bCompareResult=1;
                               cptTx=SIR_TX_Device.cptTXDataBuffer;
                               cptRx=SIR_RX_Device.cptRXDataBuffer;
                               for (i=0;i<SIR_TX_Device.Length;i++) 
                                   {
                                    if (*(cptTx+i)!=*(cptRx+i))
                                        {printf(">>> Error (TX=0x%x / RX=0x%x)\n",(*(cptTx+i)),(*(cptRx+i)));
                                         bCompareResult=0;
                                         dwFailCounter++;
                                         while(1);
                                   	  }
                                   }	                        
                         
                             printf(">>> Loop %d ,P1->P2, SIP=%d/Tri=%d/TX_Inv=%d => PASS\n",dwFreeRunCounter,b,c,d);
                        
                          bRole=1;                
                         }
 
                     }// for (j=0;j<2;j++)//Role change
 
 
 
 
 
    	                        	         
    	         	
    	         }	
    	    
    	    }
    	}
    }	



}
while(bFreeRun==1);

}



//************************************** SIR DMA Test *******************************************

//=======================================================================================
// Function Name: SIR_Test_RreeRun_DMA
// Description: a= BAUD Rate
//              b= SIP
//              c= Tri Level
//              d= TX Pulse Inverse
//======================================================================================
void SIR_Test_RreeRun_DMA(void)
{

char *cptTx,*cptRx;
int i;
UINT8 a,b,c,d,bCompareResult,bFreeRun,dwFailCounter;
UINT32 dwFreeRunCounter,dwAPB_DMA_Counter;

   printf("Free Run(1/0->Y/N): ");
   scanf("%d",&bFreeRun);
   printf("\n");

SIR_Test_Patern_Init();

dwFailCounter=0;
dwFreeRunCounter=0;
do{

for (a=0;a<6;a++) //for (a=0;a<6;a++)
    {
    switch (a)
           {
           	case 0:
           	     printf("*** SIR-DMA, Size = %d ,BR = 115200 Testing...\n",SIR_TX_Device.Length);
           	break;
           	case 1:
           	     printf("*** SIR-DMA, Size = %d ,BR = 57600 Testing...\n",SIR_TX_Device.Length);
           	break;
           	case 2:
           	     printf("*** SIR-DMA, Size = %d ,BR = 38400 Testing...\n",SIR_TX_Device.Length);

           	break;
           	case 3:
           	     printf("*** SIR-DMA, Size = %d ,BR = 19200 Testing...\n",SIR_TX_Device.Length);

           	break;
           	case 4:
           	     printf("*** SIR-DMA, Size = %d ,BR = 14400 Testing...\n",SIR_TX_Device.Length);

           	break;
           	case 5:
           	     printf("*** SIR-DMA, Size = %d ,BR = 9600 Testing...\n",SIR_TX_Device.Length);

           	break;

           	}



    for (b=0;b<2;b++)	//2
    	{
    	 for(c=0;c<4;c++) //4
    	    {
    	     //for (d=0;d<2;d++)//2
    	       //  {
    	         //====================== Port1 -> Port2 ====================================
    	         d=0;
    	         dwFreeRunCounter++;
    	          //1.Init TX =>port1
                      fLib_SIR_TX_Init_DMA(CPE_UART1_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_Test_Patern.TX_Pules_Inverse[d]);
                  //2.Init RX =>port2
                      fLib_SIR_RX_Init_DMA(CPE_UART2_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_TEST_RX_DATA_BASE_ADDR,0,SIR_TX_Device.Length);
                  //3.Send data
                      fLib_SIR_TX_Data_DMA(SIR_TX_Device.port,SIR_TX_Device.cptTXDataBuffer, SIR_TX_Device.Length);
                  //4.Receive data =>Polling the APB DMA Counter number
                      do {
                      	  dwAPB_DMA_Counter=inw(AHB2APB_DMA_B+AHB2APB_DMA_CYCLE);
                      	
                      	  }while(dwAPB_DMA_Counter>0);
                  
                  
                  //5.Close device
                      fLib_SIR_TXRX_Close_DMA();
                  
                  //6.Compare data 
                      bCompareResult=1;
                      cptTx=SIR_TX_Device.cptTXDataBuffer;
                      cptRx=SIR_RX_Device.cptRXDataBuffer;
                     for (i=0;i<SIR_TX_Device.Length;i++) 
                         {
                          if (*(cptTx+i)!=*(cptRx+i))
                              {printf(">>> Error (TX=0x%x / RX=0x%x)\n",(*(cptTx+i)),(*(cptRx+i)));
                               bCompareResult=0;
                               dwFailCounter++;
                               while(1);
                         	  }
                         }	        
                     for (i=0;i<SIR_TX_Device.Length;i++) 
                         {
                          *(cptRx+i)=0x00;

                         }	        


                 // if (bFreeRun==0)   
                     if (bCompareResult==1)
                        printf(">>> Loop %d ,P1->P2 Parameter: SIP=%d/Tri=%d/TX_Inv=%d => PASS\n",dwFreeRunCounter,b,c,d);

    	         //====================== Port2 -> Port1 ====================================
                   dwFreeRunCounter++;
     	          //1.Init TX =>port1
                      fLib_SIR_TX_Init_DMA(CPE_UART2_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_Test_Patern.TX_Pules_Inverse[d]);
                  //2.Init RX =>port2
                      fLib_SIR_RX_Init_DMA(CPE_UART1_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_TEST_RX_DATA_BASE_ADDR,0,SIR_TX_Device.Length);
                  //3.Send data
                      fLib_SIR_TX_Data_DMA(SIR_TX_Device.port,SIR_TX_Device.cptTXDataBuffer, SIR_TX_Device.Length);
                  //4.Receive data =>Polling the APB DMA Counter number
                      do {
                      	  dwAPB_DMA_Counter=inw(AHB2APB_DMA_A+AHB2APB_DMA_CYCLE);
                      	
                      	  }while(dwAPB_DMA_Counter>0);
                  
                  //5.Close device
                      fLib_SIR_TXRX_Close_DMA();
                  
                  //6.Compare data 
                      bCompareResult=1;
                      cptTx=SIR_TX_Device.cptTXDataBuffer;
                      cptRx=SIR_RX_Device.cptRXDataBuffer;
                     for (i=0;i<SIR_TX_Device.Length;i++) 
                         {
                          if (*(cptTx+i)!=*(cptRx+i))
                              {printf(">>> Error (TX=0x%x / RX=0x%x)\n",(*(cptTx+i)),(*(cptRx+i)));
                               bCompareResult=0;
                               dwFailCounter++;
                                while(1);
                         	  }
                         }	        
                     for (i=0;i<SIR_TX_Device.Length;i++) 
                         {
                          *(cptRx+i)=0x00;

                         }	        


                 //  if (bFreeRun==0) 
                       if (bCompareResult==1)
                           printf(">>> Loop %d ,P2->P1 Parameter: SIP=%d/Tri=%d/TX_Inv=%d => PASS\n",dwFreeRunCounter,b,c,d);
                               
                                        	         
    	         	
    	     //    }	
    	    
    	    }
    	}
    }	



}
while(bFreeRun==1);

}
*/

//=======================================================================================
// Function Name: SIR_Test_RreeRun_PIO
// Description: a= BAUD Rate
//              b= SIP
//              c= Tri Level
//              d= TX Pulse Inverse
//======================================================================================
void SIR_Test_RreeRun_PIO(void)
{

char *cptTx,*cptRx;
int i;
UINT8 a,b,c,d,bCompareResult,bFreeRun,dwFailCounter;
UINT32 dwFreeRunCounter;

   printf("Free Run(1/0->Y/N): ");
   scanf("%d",&bFreeRun);
   printf("\n");


SIR_Test_Patern_Init();



dwFailCounter=0;
dwFreeRunCounter=0;
do{

for (a=0;a<6;a++) 
    {
    switch (a)
           {
           	case 0:
           	     printf("*** SIR-PIO, Size = %d ,BR = 115200 Testing...\n",SIR_TX_Device.Length);
            	break;
           	case 1:
           	     printf("*** SIR-PIO, Size = %d ,BR = 57600 Testing...\n",SIR_TX_Device.Length);
           	break;
           	case 2:
           	     printf("*** SIR-PIO, Size = %d ,BR = 38400 Testing...\n",SIR_TX_Device.Length);
           	break;
           	case 3:
           	     printf("*** SIR-PIO, Size = %d ,BR = 19200 Testing...\n",SIR_TX_Device.Length);
           	break;
           	case 4:
           	     printf("*** SIR-PIO, Size = %d ,BR = 14400 Testing...\n",SIR_TX_Device.Length);
           	break;
           	case 5:
           	     printf("*** SIR-PIO, Size = %d ,BR = 9600 Testing...\n",SIR_TX_Device.Length);
           	break;

           	}



    for (b=0;b<2;b++)	
    	{
    	 for(c=0;c<4;c++)
    	    {
    	     for (d=0;d<2;d++)
    	         {
    	         //====================== Port1 -> Port2 ====================================
    	         dwFreeRunCounter++;
    	          //1.Init TX =>port1
                      fLib_SIR_TX_Init(CPE_UART1_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_Test_Patern.TX_Pules_Inverse[d]);
                  //2.Init RX =>port2
                      fLib_SIR_RX_Init(CPE_UART2_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_TEST_RX_DATA_BASE_ADDR,0);
                  //3.Send data
                      fLib_SIR_TX_Data(SIR_TX_Device.port,SIR_TX_Device.cptTXDataBuffer, SIR_TX_Device.Length);
                  //4.Receive data
                      do{
                      
                        }while(SIR_RX_Device.Length<SIR_TX_Device.Length);
                  
                  //5.Close device
                      fLib_SIR_TXRX_Close();
                  
                  //6.Compare data 
                      bCompareResult=1;
                      cptTx=SIR_TX_Device.cptTXDataBuffer;
                      cptRx=SIR_RX_Device.cptRXDataBuffer;
                     for (i=0;i<SIR_TX_Device.Length;i++) 
                         {
                          if (*(cptTx+i)!=*(cptRx+i))
                              {printf(">>> Error (TX=0x%x / RX=0x%x)\n",(*(cptTx+i)),(*(cptRx+i)));
                               bCompareResult=0;
                               dwFailCounter++;
                               while(1);
                         	  }
                         }	        
                  //if (bFreeRun==0)   
                     if (bCompareResult==1)
                        printf(">>> Loop %d ,P1->P2, SIP=%d/Tri=%d/TX_Inv=%d => PASS\n",dwFreeRunCounter,b,c,d);

    	         //====================== Port2 -> Port1 ====================================
                   dwFreeRunCounter++;
     	          //1.Init TX =>port1
                      fLib_SIR_TX_Init(CPE_UART2_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_Test_Patern.TX_Pules_Inverse[d]);
                  //2.Init RX =>port2
                      fLib_SIR_RX_Init(CPE_UART1_BASE, SIR_Test_Patern.BaudRate[a],SIR_Test_Patern.SIP[b],SIR_Test_Patern.Tri_Level[c],SIR_TEST_RX_DATA_BASE_ADDR,0);
                  //3.Send data
                      fLib_SIR_TX_Data(SIR_TX_Device.port,SIR_TX_Device.cptTXDataBuffer, SIR_TX_Device.Length);
                  //4.Receive data
                      do{
                      
                        }while(SIR_RX_Device.Length<SIR_TX_Device.Length);
                  
                  //5.Close device
                      fLib_SIR_TXRX_Close();
                  
                  //6.Compare data 
                      bCompareResult=1;
                      cptTx=SIR_TX_Device.cptTXDataBuffer;
                      cptRx=SIR_RX_Device.cptRXDataBuffer;
                     for (i=0;i<SIR_TX_Device.Length;i++) 
                         {
                          if (*(cptTx+i)!=*(cptRx+i))
                              {printf(">>> Error (TX=0x%x / RX=0x%x)\n",(*(cptTx+i)),(*(cptRx+i)));
                               bCompareResult=0;
                               dwFailCounter++;
                                while(1);
                         	  }
                         }	        
                  // if (bFreeRun==0) 
                       if (bCompareResult==1)
                           printf(">>> Loop %d ,P2->P1, SIP=%d/Tri=%d/TX_Inv=%d => PASS\n",dwFreeRunCounter,b,c,d);
                               
                                        	         
    	         	
    	         }	
    	    
    	    }
    	}
    }	



}
while(bFreeRun==1);

}



