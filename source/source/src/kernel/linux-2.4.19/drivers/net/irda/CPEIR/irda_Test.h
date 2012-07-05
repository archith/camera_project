/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name: irda_FIR_Test.h                                                    *
* Description: IRDA test program                                           *
* Author: Bruce                                                            *
* Date: 2002/10/15                                                         *
* Version:1.0                                                              *
*--------------------------------------------------------------------------*
****************************************************************************/

//=============== Option ==========================================
#define IRDA_USE_LED_MODULE
#define IRDA_FIR_TEST_DUAL_DIR
#define IRDA_FIR_AUTO_COMPARE_DATA
#define IRDA_ENABLE_CACHE  
#define RX_Polling_Enable           		0x00


#define FIR_TEST_RX_ERROR_OVERRUN		    0x01
#define FIR_TEST_TX_ERROR_UNDERRUN		    0x01
#define TEST_ITEM6_ERR_PHY	                0x01
#define TEST_ITEM6_ERR_CRC                  0x02
#define TEST_ITEM6_ERR_SIZE                 0x04

//================ SIR Free Run Definition =============================
#define SIR_TEST_TX_DATA_BASE_ADDR          0x05000000
#define SIR_TEST_RX_DATA_BASE_ADDR          0x05800000
#define SIR_TEST_TXRX_SIZE                  10 //Total=(SIR_TEST_TXRX_SIZE * 256)Bytes


//================ SIR Definition =========================================
#define SIR_PW_316BRATE					0x0
#define SIR_PW_163US					0x1


//================ Struct define =======================================
typedef struct
{
	UINT8  bSetEOTMethodEnable;
	UINT8  bForceAbortEnable;
	UINT8  SIPSentByCPU;
	UINT8  PreambleLength;
	UINT8  ErrorConditionSetting;	
	UINT32 ErrorCondition_SIZE_RX_MaxLen;		
	UINT8  TXFIFO_UnderrunTestItemEnable;
	UINT8  RXFIFO_OverrunTestItemEnable;
	UINT8  InvertedPulseEnable_TX;
	UINT8  InvertedPulseEnable_RX;	
	UINT8  FIRDMATestEnable;
	UINT8  STatusFIFO_TimeoutTestEnable;
	UINT8  STatusFIFO_TimeoutTestEnable_STFF_Tri_Level;		
	UINT8  DMA_ISR_Tri_Serviced;
	UINT8  DMA_ISR_Timeout_Serviced;
	UINT16 wExpect_AutoRunCounter;
	UINT16 wAutoRunCounter;
	UINT8  Test_Auto_Run_Enable;
}FIR_TEST_ITEM_OPTION_STRUCTURE;

typedef struct
{
	UINT32  DMA_RECEIVE_ISR;
    UINT32  PASS_Counter;
}IRDA_DEBUG_STRUCTURE;

typedef struct
{
	UINT32  BaudRate[6]; //
    UINT32  SIP[2];
    UINT32  Tri_Level[4];    
    UINT32  TX_Pules_Inverse[2];    
    
}IRDA_SIT_TEST_PATERN_STRUCTURE;


//================ Extern Variables Definition =========================================
extern IRDA_SIT_TEST_PATERN_STRUCTURE    SIR_Test_Patern;
extern IRDA_FIR_TX_Config_Status         IrDA_TX_Status;
extern IRDA_FIR_TX_Data_Buffer_Status    SendData;
extern IRDA_FIR_RX_Config_Status         IrDA_RX_Status;
extern IRDA_FIR_RX_Data_Buffer_Status    ReceiveData;
extern FIR_TEST_ITEM_OPTION_STRUCTURE    TestItem;
extern UINT8                             TestBuffer[TX_BUFFER_SIZE];
extern UINT8                             ReceiveBuffer[RX_BUFFER_SIZE];
extern UINT8                             ReceiveBufferDMA[RX_BUFFER_SIZE];
extern int                               SelectTXRX;
extern UINT8                             ISR_Serviced; 
extern FIR_TEST_SELECT_PORT_STRUCTURE    SelectPort;
extern UINT8                             RX_FMIIR_ISR_Serviced; 
extern char                              IRDA_Test_Data[];
//================irda_SIR_Test.C=========================================
extern void main_SIR(void);
extern void SIRTest_FileTransTest(void);
extern void SIR_Test_RreeRun_PIO(void);
extern void SIR_Test_RreeRun_DMA(void);
extern void SIR_Test_RreeRun_PIO_2(void);
//================irda_FIR_Test1.C=========================================
extern void IRDA_FIR_Receive_Polling(void);
extern void IRDA_ISR_RX_FMIIR(void);
extern void IRDA_ISR_RX_FMLSR(void);
extern void IRDA_ISR_TX_FMIIR(void);
extern void IRDA_ISR_TX_FMLSR(void);
extern void main_FIR(void);
extern void Set_Tx_Status(UINT16 wSendByteNumber,UINT16 wEqualizeSize,UINT32 dwPortAddress,UINT8 bIRQNumber_FMIIR,UINT8 bIRQNumber_FMLSR);
extern void FIR_Send_1_Block_Data(void);
extern void Set_Rx_Status(UINT32 dwPortAddress,UINT8 bIRQnumber_FMIIR,UINT8 bIRQnumber_FMLSR);
extern void Enable_RX_Device(UINT32 port);
extern void Init_Rx_Buffer(void);
extern void Init_Tx_Buffer(UINT16 wSendByteNumber,INT8 * bpTestBuffer);
extern void FIR_Test1_Frame_Length(void);
extern void FIR_Test1_Frame_Length_100(UINT16 wSendByteNumber);
extern void FIR_Fill_TX_Buffer(void);
extern void FIR_TX_Reuse_Init(UINT32 port);
extern void FIR_RX_Reuse_Init(UINT32 port);
extern UINT8  FIR_Test1_Multi_Frame_Test(UINT16 wSendByteNumber,UINT16 wEqualizeSize);
extern void FIR_Delay(UINT16 wDelayCounter);
extern void FIR_Test1_Main(void);
extern void FIR_Test1_AutoTest_Main(void);
extern void Test_Item_Init(void);
extern void Select_TX_RX(void);

//================irda_FIR_Test2.C=========================================
extern void FIR_Test2_Set_Tx_Status(UINT16 wSendByteNumber,UINT32 dwPortAddress,UINT8 bIRQNumber_FMIIR,UINT8 bIRQNumber_FMLSR);
extern void FIR_Test2_Send_1_Block_Data(void);
extern void FIR_Test2_Set_Rx_Status(UINT32 dwPortAddress,UINT8 bIRQnumber_FMIIR,UINT8 bIRQnumber_FMLSR);
extern void FIR_Test2_Main_EOT(void);
extern void FIR_Test2_Set_EOT_Send(UINT16 wSendByteNumber);
extern void FIR_Test3_Main_ForceAbort(void);
extern void FIR_Test3_Send(void);
extern void FIR_Test3_Force_Abort_RX_TX(UINT16 wSendByteNumber,UINT16 wEqualizeSize);
extern void FIR_Test4_Main_SIP(void);
extern void FIR_Test5_Main_Preamble(void);
extern void FIR_Test6_Main_ErrorCondition(void);
extern void FIR_Test7_Main_Underrun(void);
extern void FIR_Test8_Main_Overrun(void);
extern void FIR_Test9_Main(void);

//================irda_FIR_Test3.C=========================================
extern void FIR_Test10_Main_DMA(void);
extern void FIR_Test11_Main_DMA_STFF_Timeout(void);
extern void FIR_Test10_DMA_Test(UINT16 wSendByteNumber,UINT16 wEqualizeSize);
extern void FIR_Test12_Main_Auto_Free_Run_PIO(void);
extern void FIR_Test12_Main_Auto_Free_Run_DMA(void);
extern void FIR_Auto_Disable_Interrupt(void);




