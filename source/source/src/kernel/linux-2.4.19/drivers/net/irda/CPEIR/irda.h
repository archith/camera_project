//#define PAUL_DEBUG
/****************************************************************************
* Porting to Linux on 20030822											   *
* Author: Paul Chiang													   *
* Version: 0.1								  							   * 
* History: 								  								   *
*          0.1 new creation						   						   *
* Todo: 																   *
****************************************************************************/
#ifndef CPE_IRDA_H
#define CPE_IRDA_H
#include <linux/types.h>
#include <linux/config.h>
//#include <stdio.h>
//#include <Stdlib.h>

#define printf(a...)	printk("IRDA:" ##a)

#ifdef CONFIG_CPE120_PLATFORM
#define         CPE_IRDA_BASE   CPE_UART3_BASE
#define         CPE_IRDA_INT1   IRQ_UART3_IRDA1 
#define         CPE_IRDA_INT2   IRQ_UART3_IRDA2 
#define         CPE_IRDA_DMA    2 
#else
#define 	CPE_IRDA_BASE 	CPE_UART2_BASE 
#define 	CPE_IRDA_INT1	IRQ_UART2
#define 	CPE_IRDA_INT2	IRQ_IRDA2 
#define 	CPE_IRDA_DMA	2
#endif

#define RX_BUFF_SIZE	(63*1024)
#define TX_BUFF_SIZE	(63*1024)
#define TX_TRIG_LEVEL	2
#define RX_TRIG_LEVEL	2

#define SIR_TX_TIMEOUT	400000//200000//50000

#ifdef PAUL_DEBUG
	#define P_DEBUG(a...)		printk("Paul:" ##a)
	#define P_DEBUG_SIR(a...)	printk("Paul:" ##a)
#else
	#define P_DEBUG(a...)	
	#define P_DEBUG_SIR(a...)	udelay(1000)
#endif	

#define IRDA_ENABLE_CACHE 
//============================ Baund Rate  Mapping =============================================
#define IRDA_CLK                     	    48000000//22118000//48000000//18432000/2	
#define PSR_CLK				                1843200
//#define FIFO16_ENABLE                       0x01  
//#define FIFO32_ENABLE                       0x01  
#define FIFO64_ENABLE                       0x01 
#define OUT3_USE_BIT6                       0x01

//CPE_BAUD_115200    can't divide
#define IRDA_BAUD_115200                    (int)(PSR_CLK / 1843200)//1
#define IRDA_BAUD_57600                     (int)(PSR_CLK / 921600) //2
#define IRDA_BAUD_38400			            (int)(PSR_CLK / 614400) //3
#define IRDA_BAUD_19200                     (int)(PSR_CLK / 307200) //6
#define IRDA_BAUD_14400                     (int)(PSR_CLK / 230400) //8
#define IRDA_BAUD_9600                      (int)(PSR_CLK / 153600) //12
#define IRDA_BAUD_4800                      (int)(PSR_CLK / 76800)  //24
#define IRDA_BAUD_2400                      (int)(PSR_CLK / 38400)  //48
#define IRDA_BAUD_1200                      (int)(PSR_CLK / 19200)  //96

//============================ Register Address Mapping =========================================

//Registre Address Define (IrDA/FIR)
#define SERIAL_TST      			        0x14
#define SERIAL_FMIIR_DMA			        0x3C
#define SERIAL_FMIIR  			            0x3C
#define SERIAL_FMIIER				        0x40
#define SERIAL_STFF_STS				        0x44
#define SERIAL_STFF_RXLENL			        0x48
#define SERIAL_STFF_RXLENH			        0x4c
#define SERIAL_FMLSR				        0x50
#define SERIAL_FMLSIER				        0x54
#define SERIAL_RSR				            0x58
#define SERIAL_RXFF_CNTR			        0x5c
#define SERIAL_LSTFMLENL			        0x60
#define SERIAL_LSTFMLENH			        0x64

//=========================== Register Value/Bit Define ==========================================

//Register "PSR=0x08(DLAB=1)" 
#define SERIAL_PSR_FIR_FIX_VALUE		    0x06


//Register "FCR=0x08" Define (IrDA/FIR)
#define SERIAL_FCR_TX_TRIGGER16_LEVEL_1		0x01 
#define SERIAL_FCR_TX_TRIGGER16_LEVEL_3		0x03 
#define SERIAL_FCR_TX_TRIGGER16_LEVEL_9		0x09 
#define SERIAL_FCR_TX_TRIGGER16_LEVEL_13	0x0D 
#define SERIAL_FCR_RX_TRIGGER16_LEVEL_1		0x01 
#define SERIAL_FCR_RX_TRIGGER16_LEVEL_4		0x04 
#define SERIAL_FCR_RX_TRIGGER16_LEVEL_8		0x08 
#define SERIAL_FCR_RX_TRIGGER16_LEVEL_14	0x0E 

#define SERIAL_FCR_TX_TRIGGER32_LEVEL_1		0x01 
#define SERIAL_FCR_TX_TRIGGER32_LEVEL_6		0x06 
#define SERIAL_FCR_TX_TRIGGER32_LEVEL_18	0x12 
#define SERIAL_FCR_TX_TRIGGER32_LEVEL_26	0x1A 
#define SERIAL_FCR_RX_TRIGGER32_LEVEL_1		0x01 
#define SERIAL_FCR_RX_TRIGGER32_LEVEL_8		0x08 
#define SERIAL_FCR_RX_TRIGGER32_LEVEL_16	0x10 
#define SERIAL_FCR_RX_TRIGGER32_LEVEL_28	0x1C 

#define SERIAL_FCR_TX_TRIGGER64_LEVEL_1		1 
#define SERIAL_FCR_TX_TRIGGER64_LEVEL_16	16 
#define SERIAL_FCR_TX_TRIGGER64_LEVEL_32	32 
#define SERIAL_FCR_TX_TRIGGER64_LEVEL_56	56 
#define SERIAL_FCR_RX_TRIGGER64_LEVEL_1		1 
#define SERIAL_FCR_RX_TRIGGER64_LEVEL_16	16 
#define SERIAL_FCR_RX_TRIGGER64_LEVEL_32	32  
#define SERIAL_FCR_RX_TRIGGER64_LEVEL_56	56 

#ifdef FIFO32_ENABLE 
    #define FIFO_SIZE			            0x20
    #define SERIAL_FCR_TX_TRI1	            SERIAL_FCR_TX_TRIGGER32_LEVEL_1
    #define SERIAL_FCR_TX_TRI2	            SERIAL_FCR_TX_TRIGGER32_LEVEL_6
    #define SERIAL_FCR_TX_TRI3	            SERIAL_FCR_TX_TRIGGER32_LEVEL_18
    #define SERIAL_FCR_TX_TRI4	            SERIAL_FCR_TX_TRIGGER32_LEVEL_26
    #define SERIAL_FCR_RX_TRI1	            SERIAL_FCR_RX_TRIGGER32_LEVEL_1
    #define SERIAL_FCR_RX_TRI2	            SERIAL_FCR_RX_TRIGGER32_LEVEL_8
    #define SERIAL_FCR_RX_TRI3	            SERIAL_FCR_RX_TRIGGER32_LEVEL_16
    #define SERIAL_FCR_RX_TRI4	            SERIAL_FCR_RX_TRIGGER32_LEVEL_28
#endif

#ifdef FIFO64_ENABLE                             
    #define FIFO_SIZE			            0x40
    #define SERIAL_FCR_TX_TRI1	            SERIAL_FCR_TX_TRIGGER64_LEVEL_1	
    #define SERIAL_FCR_TX_TRI2	            SERIAL_FCR_TX_TRIGGER64_LEVEL_16	
    #define SERIAL_FCR_TX_TRI3	            SERIAL_FCR_TX_TRIGGER64_LEVEL_32	
    #define SERIAL_FCR_TX_TRI4	            SERIAL_FCR_TX_TRIGGER64_LEVEL_56
    #define SERIAL_FCR_RX_TRI1	            SERIAL_FCR_RX_TRIGGER64_LEVEL_1	
    #define SERIAL_FCR_RX_TRI2	            SERIAL_FCR_RX_TRIGGER64_LEVEL_16	
    #define SERIAL_FCR_RX_TRI3	            SERIAL_FCR_RX_TRIGGER64_LEVEL_32	
    #define SERIAL_FCR_RX_TRI4	            SERIAL_FCR_RX_TRIGGER64_LEVEL_56
#endif



#define SERIAL_FCR_TX_FIFO_RESET		    0x04 
#define SERIAL_FCR_RX_FIFO_RESET		    0x02 
#define SERIAL_FCR_TXRX_FIFO_ENABLE         0x01 
             
             
//Register "TST=0x10" Define     
#ifdef OUT3_USE_BIT6      
    #define SERIAL_MCR_OUT3 	                0x40
    #define SERIAL_MCR_DMA2 	                0x20    
#else
    #define SERIAL_MCR_OUT3 	                0x20
#endif
                                       
//Register "TST=0x14" Define                
#define SERIAL_TST_TEST_PAR_ERR 	        0x01
#define SERIAL_TST_TEST_FRM_ERR 	        0x02
#define SERIAL_TST_TEST_BAUDGEN 	        0x04
#define SERIAL_TST_TEST_PHY_ERR       	    0x08
#define SERIAL_TST_TEST_CRC_ERR	    	    0x10
                          
                                            
//Register "MDR=0x20" Define                
#define SERIAL_MDR_IITx				        0x40
#define SERIAL_MDR_FIRx				        0x20
#define SERIAL_MDR_MDSEL			        0x1c
#define SERIAL_MDR_DMA_ENABLE          	    0x10
#define SERIAL_MDR_FMEND_ENABLE			    0x08
#define SERIAL_MDR_SIP_BY_CPU			    0x04
#define SERIAL_MDR_UART_MODE			    0x00
#define SERIAL_MDR_SIR_MODE			        0x01
#define SERIAL_MDR_FIR_MODE			        0x02
#define SERIAL_MDR_TX_INV			        0x40
#define SERIAL_MDR_RX_INV			        0x20
//Register "ACR-0x24" Define (IrDA/FIR)
#define SERIAL_ACR_TX_ENABLE			    0x01
#define SERIAL_ACR_RX_ENABLE			    0x02
#define SERIAL_ACR_FIR_SET_EOT			    0x04
#define SERIAL_ACR_FORCE_ABORT			    0x08
#define SERIAL_ACR_SEND_SIP			        0x10
#define SERIAL_ACR_STFF_TRGL_1			    0x01
#define SERIAL_ACR_STFF_TRGL_4			    0x04
#define SERIAL_ACR_STFF_TRGL_7			    0x07
#define SERIAL_ACR_STFF_TRGL_8			    0x08
                                            
#define SERIAL_ACR_STFF_TRG1			    0x00
#define SERIAL_ACR_STFF_TRG2			    0x20
#define SERIAL_ACR_STFF_TRG3			    0x40
#define SERIAL_ACR_STFF_TRG4			    0x60
#define SERIAL_ACR_STFF_CLEAR			    0x9F                                            
                                            
                                            
#define SIR_ACR_SIR_PW_316BRATE			    0x00
#define SIR_ACR_SIR_PW_163US			    0x80

//Register "TXLENL/TXLENH=0x28/0x2C" Define (IrDA/FIR)
#define SERIAL_TXLENL_DEFAULT			    0x00FF

//Register "MRXLENL/MRXLENH=0x30/0x34" Define (IrDA/FIR)
#define SERIAL_MRXLENHL_DEFAULT			    0x1F40


//Register "PLR=0x38" Define (IrDA/FIR)
#define SERIAL_PLR_16    			        0x00
#define SERIAL_PLR_4  			            0x01
#define SERIAL_PLR_8    			        0x02
#define SERIAL_PLR_32  			            0x03

//Register "FMIIER/PIO=0x40" Define (IrDA/FIR)
#define SERIAL_FMIIR_PIO_FRM_SENT		    0x20
#define SERIAL_FMIIR_PIO_EOF_DETECTED	    0x10
#define SERIAL_FMIIR_PIO_TXFIFO_URUN	    0x08
#define SERIAL_FMIIR_PIO_RXFIFO_ORUN	    0x04
#define SERIAL_FMIIR_PIO_TXFIFO_TRIG	    0x02
#define SERIAL_FMIIR_PIO_RXFIFO_TRIG	    0x01
#define SERIAL_FMIIR_PIO_RX_EVENT		    0x15


#define SERIAL_FMIIR_DMA_FRM_SENT		    0x20
#define SERIAL_FMIIR_DMA_TXFIFO_URUN		0x08
#define SERIAL_FMIIR_DMA_STFIFO_ORUN		0x04
#define SERIAL_FMIIR_DMA_STFIFO_TIMEOUT	    0x02
#define SERIAL_FMIIR_DMA_STFIFO_TRIG		0x01
#define SERIAL_FMIIR_DMA_RX_EVENT		    0x07


//Register "FMIIER/PIO=0x40" Define (IrDA/FIR)
#define SERIAL_FMIIER_PIO_FRM_SENT		    0x20
#define SERIAL_FMIIER_PIO_EOF_DETECTED		0x10
#define SERIAL_FMIIER_PIO_TXFIFO_URUN		0x08
#define SERIAL_FMIIER_PIO_RXFIFO_ORUN		0x04
#define SERIAL_FMIIER_PIO_TXFIFO_TRIG		0x02
#define SERIAL_FMIIER_PIO_RXFIFO_TRIG		0x01

#define SERIAL_FMIIER_DMA_FRM_SENT		    0x20
#define SERIAL_FMIIER_DMA_TXFIFO_URUN		0x08
#define SERIAL_FMIIER_DMA_STFIFO_ORUN		0x04
#define SERIAL_FMIIER_DMA_STFIFO_TIMEOUT	0x02
#define SERIAL_FMIIER_DMA_STFIFO_TRIG		0x01



//Register "STFF_STS=0x44" Define (IrDA/FIR)
#define SERIAL_STFF_STS_RXFIFO_ORUN		    0x01
#define SERIAL_STFF_STS_CRC_ERR			    0x02
#define SERIAL_STFF_STS_PHY_ERR			    0x04
#define SERIAL_STFF_STS_SIZE_ERR		    0x08
#define SERIAL_STFF_STS_STS_VLD			    0x10


//Register "FMLSR=0x50" Define (IrDA/FIR)
#define SERIAL_FMLSR_FIR_IDLE			    0x80
#define SERIAL_FMLSR_TX_EMPTY			    0x40
#define SERIAL_FMLSR_STFIFO_FULL		    0x20
#define SERIAL_FMLSR_SIZE_ERR			    0x10
#define SERIAL_FMLSR_PHY_ERROR		        0x08
#define SERIAL_FMLSR_CRC_ERROR		        0x04
#define SERIAL_FMLSR_STFIFO_EMPTY		    0x02
#define SERIAL_FMLSR_RXFIFO_EMPTY		    0x01

//Register "FMLSIER=0x54" Define (IrDA/FIR)
#define SERIAL_FMLSIER_FIR_IDLE			    0x80
#define SERIAL_FMLSIER_TX_EMPTY			    0x40
#define SERIAL_FMLSIER_STFIFO_FULL		    0x20
#define SERIAL_FMLSIER_SIZE_ERR			    0x10
#define SERIAL_FMLSIER_PHY_ERROR		    0x08
#define SERIAL_FMLSIER_CRC_ERROR		    0x04
#define SERIAL_FMLSIER_STFIFO_EMPTY		    0x02
#define SERIAL_FMLSIER_RXFIFO_EMPTY		    0x01

//=========================== Send Data Define ==========================================
#define SERIAL_FIR_FRAME_MAX_SIZE		    2000
#define SERIAL_FIR_FRAME_MAX_NUMBER		    7
#define TX_BUFFER_SIZE		                (40*256) 

//=========================== Receive Data Define ==========================================
#define RX_BUFFER_SIZE		                10000 

//=========================== Error Type Definition ==========================================
#define SERIAL_FIR_RX_ERROR_OVERRUN		    0x01
#define SERIAL_FIR_TX_ERROR_UNDERRUN		0x02

//=========================== Structure Define ==========================================
typedef struct
{
	UINT8  PSR;
	UINT8  DLL;
	UINT8  DLM;
	UINT8  MDR;
	UINT8  PLR;
	UINT32 TXLENHL;
	UINT32 LSTFMLENHL;
	UINT8  LST_FrameNumber;
	UINT8  TX_FIFO_Trigger_Level;
	UINT8  TX_FMIIER;
	UINT8  TX_FMLSIER;
	UINT32 TX_PORT_ADDRESS;	
	UINT8  IRQNumber_FMIIR;
	UINT8  IRQNumber_FMLSR;	
	UINT16 SET_EOT_FRAME_LENGTH;
	UINT16 SET_EOT_LAST_FRAME_LENGTH;
	
	UINT32 DMA_RegisterPort;
	UINT32 DMA_Request_Select;
	
}IRDA_FIR_TX_Config_Status;


typedef struct
{
	UINT8  PSR;
	UINT8  DLL;
	UINT8  DLM;
	UINT8  PLR;	
	UINT8  MDR; //Select FIR/DMA mode
	UINT32 MRXLENHL; //maximum length
	UINT8  RX_FIFO_Trigger_Level;
	UINT8  ST_FIFO_Trigger_Level;
	UINT8  RX_FMIIER;
	UINT8  RX_FMLSIER;
	UINT32 RX_PORT_ADDRESS;	
	UINT8  IRQNumber_FMIIR;	
	UINT8  IRQNumber_FMLSR;		
	UINT32 DMA_RegisterPort;
	UINT32 DMA_Request_Select;			
}IRDA_FIR_RX_Config_Status;


typedef struct
{
	UINT8 *bpTXDataBuffer;
	UINT32 RemainByte;
	UINT32 TotalByte;
	UINT8  TX_Expect_Frame_Numbers;
	UINT16 TX_1Frame_Bytes;
	UINT8  Send_FRM_SENT_OK_Counter;
	UINT8  SendErrorType;
}IRDA_FIR_TX_Data_Buffer_Status;

typedef struct
{
	UINT32 Len;
	UINT8  STFFStatus;
   	
}IRDA_FIR_RX_DMA_Status;

			
typedef struct
{
	UINT8  *bpRXDataBuffer;
	UINT8  RX_Receive_Frame_Counter;
	UINT32 ReceivedBytes;
	UINT8  RX_Expect_Frame_Numbers;
	UINT8  ReceiveErrorType_FMIIR;
	UINT8  ReceiveErrorType_FMLSR;	
	UINT8  ReceiveStatusFIFO_Number;	
	IRDA_FIR_RX_DMA_Status DMA_Buffer[10]; //Reserve 20 status fifo

}IRDA_FIR_RX_Data_Buffer_Status;
			
typedef struct
{
	UINT32 Slect_PORT_ADDRESS;	
	UINT8  Select_IRQNumber_FMIIR;
	UINT8  Select_IRQNumber_FMLSR;	
	UINT32 DMA_Reg_ADD;
	UINT32 DMA_Port;	

	
}FIR_TEST_SELECT_PORT_STRUCTURE;			
			
typedef struct
{
	UINT32  port;
	volatile UINT32  Length;
	char    *cptRXDataBuffer;
	UINT32  IRQ_Num;
   	
}IRDA_SIR_RX_Status;
			
typedef struct
{
	UINT32  port;
	volatile UINT32  Length;
	char    *cptTXDataBuffer;
   	
}IRDA_SIR_TX_Status;		


//========================= Extern function call ===========================================
//extern int fLib_IRDA_INT_Init(UINT32 IRQ_IrDA,PrHandler function);
//extern int fLib_IRDA_INT_Disable(UINT32 IRQ_IrDA);
extern void fLib_SetFIRPrescal(UINT32 port,UINT32 PSR_Value);
extern void fLib_IRDA_Set_FCR(UINT32 port, UINT32 setmode);
extern void fLib_IRDA_Enable_FCR(UINT32 port, UINT32 setmode);
extern void fLib_IRDA_Set_FCR_Trigl_Level(UINT32 port, UINT32 Tx_Tri_Level , UINT32 Rx_Tri_Level );
extern void fLib_IRDA_Set_FCR_Trigl_Level_2(UINT32 port, UINT32 Tx_Tri_Level , UINT32 Rx_Tri_Level );
extern void fLib_IRDA_Set_TST(UINT32 port,UINT8 setmode);
extern void fLib_IRDA_Set_MDR(UINT32 port,UINT8 setmode);
extern void fLib_IRDA_Enable_MDR(UINT32 port,UINT8 setmode);
extern void fLib_IRDA_Disable_MDR(UINT32 port,UINT8 setmode);
extern void fLib_IRDA_Set_ACR(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Enable_ACR(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Disable_ACR(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Set_ACR_STFIFO_Trigl_Level(UINT32 port, UINT32 STFIFO_Tri_Level);
extern void fLib_IRDA_Set_TXLEN_LSTFMLEN(UINT32 port,UINT32 Normal_FrmLength,UINT32 Last_FrmLength,UINT32 FrmNum);
extern void fLib_IRDA_Set_MaxLen(UINT32 port,UINT32 Max_RxLength);
extern void fLib_IRDA_Set_PLR(UINT32 port,UINT32 Preamble_Length);
extern UINT8 fLib_IRDA_Read_FMIIR(UINT32 port);
extern void fLib_IRDA_Set_FMIIER(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Enable_FMIIER(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Disable_FMIIER(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Set_FMLSIER(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Enable_FMLSIER(UINT32 port,UINT32 mode);
extern void fLib_IRDA_Disable_FMLSIER(UINT32 port,UINT32 mode);
extern UINT8 fLib_IRDA_Read_STFF_STS(UINT32 port);
extern UINT32 fLib_IRDA_Read_STFF_RXLEN(UINT32 port);
extern UINT8 fLib_IRDA_Read_RSR(UINT32 port);
extern UINT8 fLib_IRDA_Read_RXFF_CNTR(UINT32 port);
extern UINT8 fLib_IRDA_Read_FMLSR(UINT32 port);
extern UINT32 fLib_IRDA_FIR_WRITE_CHAR(UINT32 port, char Ch);
extern void fLib_SIRInit(UINT32 port, UINT32 baudrate,UINT32 num,UINT32 len);
extern void fLib_SetSIRPW(UINT32 port,UINT32 enable);
extern void fLib_IRDA_SIP_TIMER_ISR(void);
extern void fLib_IRDA_SIP_TIMER_INIT(UINT32 InitPort);
extern void fLib_IRDA_SIP_TIMER_DISABLE(void);
extern void fLib_SetIRTxInv(UINT32 port,UINT32 enable);
extern void fLib_SetIRRxInv(UINT32 port,UINT32 enable);
//================ SIR Area =====================================
extern void fLib_SIR_RX_ISR(void);
extern void fLib_SIR_TX_Init(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwTX_Tri_Value,UINT32 bInv);
extern void fLib_SIR_RX_Init(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwRX_Tri_Value,UINT32 ReceiveDataAddr,UINT32 bInv);
extern void fLib_SIR_TX_Data(UINT32 dwport,char *bpData, UINT32 dwLength);
extern void fLib_SIR_TXRX_Close(void);	
extern void fLib_SIR_TX_Close(void);
extern void fLib_SIR_RX_Close(void);
extern void fLib_SetIRTxInv(UINT32 port,UINT32 enable);

extern IRDA_SIR_RX_Status                            SIR_RX_Device;
extern IRDA_SIR_TX_Status                            SIR_TX_Device;
//extern void fLib_IrDA_AutoMode_SIR(UINT32 port);
//extern void fLib_IrDA_AutoMode_FIR(UINT32 port);
extern void fLib_IrDA_AutoMode_SIR_Low(UINT32 port);
extern void fLib_IrDA_AutoMode_FIR_High(UINT32 port);

extern void fLib_SIR_RX_Init_DMA(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwRX_Tri_Value,UINT32 ReceiveDataAddr,UINT32 bInv,UINT32 dwReceiveDataLength);
extern void fLib_SIR_TX_Init_DMA(UINT32 port, UINT32 baudrate,UINT32 SIP_PW_Value, UINT32 dwTX_Tri_Value,UINT32 bInv);
extern void fLib_SIR_TX_Data_DMA(UINT32 dwport,char *bpData, UINT32 dwLength);
extern void fLib_SIR_TXRX_Close_DMA(void);

//paulong add 20030825
extern int fLib_SetSIRSpeed(u32 port,__u32 speed);	
extern void fLib_SetIRMode(u32 port,u32 mode);
extern void fLib_SetFIFO(u32 port);
extern int fLib_irda_probe(u32 iobase);
extern void DumpBuff(char *buff,u32 count);


#endif	//#ifndef CPE_IRDA_H
