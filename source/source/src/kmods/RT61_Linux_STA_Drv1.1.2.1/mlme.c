/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mlme.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	John Chang	2004-08-25		Modify from RT2500 code base
	John Chang  2004-09-06      modified for RT2600
*/

#include "rt_config.h"
#include <stdarg.h>
#include <net/iw_handler.h>

// since RT61 has better RX sensibility, we have to limit TX ACK rate not to exceed our normal data TX rate.
// otherwise the WLAN peer may not be able to receive the ACK thus downgrade its data TX rate
ULONG BasicRateMask[12]            = {0xfffff001 /* 1-Mbps */, 0xfffff003 /* 2 Mbps */, 0xfffff007 /* 5.5 */, 0xfffff00f /* 11 */,
                                      0xfffff01f /* 6 */     , 0xfffff03f /* 9 */     , 0xfffff07f /* 12 */ , 0xfffff0ff /* 18 */,
                                      0xfffff1ff /* 24 */    , 0xfffff3ff /* 36 */    , 0xfffff7ff /* 48 */ , 0xffffffff /* 54 */};

UCHAR BROADCAST_ADDR[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// e.g. RssiSafeLevelForTxRate[RATE_36]" means if the current RSSI is greater than 
//      this value, then it's quaranteed capable of operating in 36 mbps TX rate in 
//      clean environment.
//                                TxRate: 1   2   5.5   11   6    9    12   18   24   36   48   54   72  100
CHAR RssiSafeLevelForTxRate[] ={  -92, -91, -90, -87, -88, -86, -85, -83, -81, -78, -72, -71, -40, -40 };

                                  //  1      2       5.5      11  
UCHAR Phy11BNextRateDownward[] = {RATE_1, RATE_1,   RATE_2,  RATE_5_5};
UCHAR Phy11BNextRateUpward[]   = {RATE_2, RATE_5_5, RATE_11, RATE_11};

                                  //  1      2       5.5      11        6        9        12      18       24       36       48       54
UCHAR Phy11BGNextRateDownward[]= {RATE_1, RATE_1,   RATE_2,  RATE_5_5,RATE_11,  RATE_6,  RATE_11, RATE_12, RATE_18, RATE_24, RATE_36, RATE_48};
UCHAR Phy11BGNextRateUpward[]  = {RATE_2, RATE_5_5, RATE_11, RATE_12, RATE_9,   RATE_12, RATE_18, RATE_24, RATE_36, RATE_48, RATE_54, RATE_54};

                                  //  1      2       5.5      11        6        9        12      18       24       36       48       54
UCHAR Phy11ANextRateDownward[] = {RATE_6, RATE_6,   RATE_6,  RATE_6,  RATE_6,   RATE_6,  RATE_9,  RATE_12, RATE_18, RATE_24, RATE_36, RATE_48};
UCHAR Phy11ANextRateUpward[]   = {RATE_9, RATE_9,   RATE_9,  RATE_9,  RATE_9,   RATE_12, RATE_18, RATE_24, RATE_36, RATE_48, RATE_54, RATE_54};

//                              RATE_1,  2, 5.5, 11,  6,  9, 12, 18, 24, 36, 48, 54
static USHORT RateUpPER[]   = {    40,  40,  35, 20, 20, 20, 20, 16, 10, 16, 10,  6 }; // in percentage
static USHORT RateDownPER[] = {    50,  50,  45, 45, 35, 35, 35, 35, 25, 25, 25, 13 }; // in percentage

UCHAR  RateIdToMbps[]    = { 1, 2, 5, 11, 6, 9, 12, 18, 24, 36, 48, 54, 72, 100};
USHORT RateIdTo500Kbps[] = { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108, 144, 200};

UCHAR   ZeroSsid[32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

UCHAR  SsidIe    = IE_SSID;
UCHAR  SupRateIe = IE_SUPP_RATES;
UCHAR  ExtRateIe = IE_EXT_SUPP_RATES;
UCHAR  ErpIe     = IE_ERP;
UCHAR  DsIe      = IE_DS_PARM;
UCHAR  TimIe     = IE_TIM;
UCHAR  WpaIe     = IE_WPA;
UCHAR  Wpa2Ie    = IE_WPA2;
UCHAR  IbssIe    = IE_IBSS_PARM;

extern UCHAR	WPA_OUI[];
extern UCHAR	RSN_OUI[];


// Reset the RFIC setting to new series    
RTMP_RF_REGS RF5225RegTable[] = {
//      ch   R1          R2          R3(TX0~4=0) R4
		{1,  0x95002ccc, 0x95004786, 0x95068455, 0x950ffa0b},
		{2,  0x95002ccc, 0x95004786, 0x95068455, 0x950ffa1f},
		{3,  0x95002ccc, 0x9500478a, 0x95068455, 0x950ffa0b},
		{4,  0x95002ccc, 0x9500478a, 0x95068455, 0x950ffa1f},
		{5,  0x95002ccc, 0x9500478e, 0x95068455, 0x950ffa0b},
		{6,  0x95002ccc, 0x9500478e, 0x95068455, 0x950ffa1f},
		{7,  0x95002ccc, 0x95004792, 0x95068455, 0x950ffa0b},
		{8,  0x95002ccc, 0x95004792, 0x95068455, 0x950ffa1f},
		{9,  0x95002ccc, 0x95004796, 0x95068455, 0x950ffa0b},
		{10, 0x95002ccc, 0x95004796, 0x95068455, 0x950ffa1f},
		{11, 0x95002ccc, 0x9500479a, 0x95068455, 0x950ffa0b},
		{12, 0x95002ccc, 0x9500479a, 0x95068455, 0x950ffa1f},
		{13, 0x95002ccc, 0x9500479e, 0x95068455, 0x950ffa0b},
		{14, 0x95002ccc, 0x950047a2, 0x95068455, 0x950ffa13},

		// 802.11 UNI / HyperLan 2
		{36, 0x95002ccc, 0x9500499a, 0x9509be55, 0x950ffa23},
		{40, 0x95002ccc, 0x950049a2, 0x9509be55, 0x950ffa03},
		{44, 0x95002ccc, 0x950049a6, 0x9509be55, 0x950ffa0b},
		{48, 0x95002ccc, 0x950049aa, 0x9509be55, 0x950ffa13},
		{52, 0x95002ccc, 0x950049ae, 0x9509ae55, 0x950ffa1b},
		{56, 0x95002ccc, 0x950049b2, 0x9509ae55, 0x950ffa23},
		{60, 0x95002ccc, 0x950049ba, 0x9509ae55, 0x950ffa03},
		{64, 0x95002ccc, 0x950049be, 0x9509ae55, 0x950ffa0b},

		// 802.11 HyperLan 2
		{100, 0x95002ccc, 0x95004a2a, 0x950bae55, 0x950ffa03},
		{104, 0x95002ccc, 0x95004a2e, 0x950bae55, 0x950ffa0b},
		{108, 0x95002ccc, 0x95004a32, 0x950bae55, 0x950ffa13},
		{112, 0x95002ccc, 0x95004a36, 0x950bae55, 0x950ffa1b},
		{116, 0x95002ccc, 0x95004a3a, 0x950bbe55, 0x950ffa23},
		{120, 0x95002ccc, 0x95004a82, 0x950bbe55, 0x950ffa03},
		{124, 0x95002ccc, 0x95004a86, 0x950bbe55, 0x950ffa0b},
		{128, 0x95002ccc, 0x95004a8a, 0x950bbe55, 0x950ffa13},
		{132, 0x95002ccc, 0x95004a8e, 0x950bbe55, 0x950ffa1b},
		{136, 0x95002ccc, 0x95004a92, 0x950bbe55, 0x950ffa23},

		// 802.11 UNII
		{140, 0x95002ccc, 0x95004a9a, 0x950bbe55, 0x950ffa03},
		{149, 0x95002ccc, 0x95004aa2, 0x950bbe55, 0x950ffa1f},
		{153, 0x95002ccc, 0x95004aa6, 0x950bbe55, 0x950ffa27},
		{157, 0x95002ccc, 0x95004aae, 0x950bbe55, 0x950ffa07},
		{161, 0x95002ccc, 0x95004ab2, 0x950bbe55, 0x950ffa0f},
		{165, 0x95002ccc, 0x95004ab6, 0x950bbe55, 0x950ffa17},

		//MMAC(Japan)J52 ch 34,38,42,46
		{34, 0x95002ccc, 0x9500499a, 0x9509be55, 0x950ffa0b},
		{38, 0x95002ccc, 0x9500499e, 0x9509be55, 0x950ffa13},
		{42, 0x95002ccc, 0x950049a2, 0x9509be55, 0x950ffa1b},
		{46, 0x95002ccc, 0x950049a6, 0x9509be55, 0x950ffa23},

};
UCHAR NUM_OF_5225_CHNL = (sizeof(RF5225RegTable) / sizeof(RTMP_RF_REGS));

// Reset the RFIC setting to new series    
RTMP_RF_REGS RF5225RegTable_1[] = {
//      ch   R1          R2          R3(TX0~4=0) R4
        {1,  0x95002ccc, 0x95004786, 0x95068455, 0x950ffa0b},
        {2,  0x95002ccc, 0x95004786, 0x95068455, 0x950ffa1f},
        {3,  0x95002ccc, 0x9500478a, 0x95068455, 0x950ffa0b},
        {4,  0x95002ccc, 0x9500478a, 0x95068455, 0x950ffa1f},
        {5,  0x95002ccc, 0x9500478e, 0x95068455, 0x950ffa0b},
        {6,  0x95002ccc, 0x9500478e, 0x95068455, 0x950ffa1f},
        {7,  0x95002ccc, 0x95004792, 0x95068455, 0x950ffa0b},
        {8,  0x95002ccc, 0x95004792, 0x95068455, 0x950ffa1f},
        {9,  0x95002ccc, 0x95004796, 0x95068455, 0x950ffa0b},
        {10, 0x95002ccc, 0x95004796, 0x95068455, 0x950ffa1f},
        {11, 0x95002ccc, 0x9500479a, 0x95068455, 0x950ffa0b},
        {12, 0x95002ccc, 0x9500479a, 0x95068455, 0x950ffa1f},
        {13, 0x95002ccc, 0x9500479e, 0x95068455, 0x950ffa0b},
        {14, 0x95002ccc, 0x950047a2, 0x95068455, 0x950ffa13},
    
        // 802.11 UNI / HyperLan 2
        {36,	0x95002cd4,	0x9504481a,	0x95098455,	0x950c0a03},
		{40,	0x95002cd0,	0x95044682,	0x95098455,	0x950c0a03},
		{44,	0x95002cd0,	0x95044686,	0x95098455,	0x950c0a1b},
		{48,	0x95002cd0,	0x9504468e,	0x95098655,	0x950c0a0b},
		{52,	0x95002cd0,	0x95044692,	0x95098855,	0x950c0a23},
		{56,	0x95002cd0,	0x9504469a,	0x95098c55,	0x950c0a13},
		{60,	0x95002cd0,	0x950446a2,	0x95098e55,	0x950c0a03},
		{64,	0x95002cd0,	0x950446a6,	0x95099255,	0x950c0a1b},

        // 802.11 HyperLan 2
        {100,	0x95002cd4,	0x9504489a,	0x950b9855,	0x950c0a03},
		{104,	0x95002cd4,	0x950448a2,	0x950b9855,	0x950c0a03},
		{108,	0x95002cd4,	0x950448aa,	0x950b9855,	0x950c0a03},
		{112,	0x95002cd4,	0x950448b2,	0x950b9a55,	0x950c0a03},
		{116,	0x95002cd4,	0x950448ba,	0x950b9a55,	0x950c0a03},
		{120,	0x95002cd0,	0x95044702,	0x950b9a55,	0x950c0a03},
		{124,	0x95002cd0,	0x95044706,	0x950b9a55,	0x950c0a1b},
		{128,	0x95002cd0,	0x9504470e,	0x950b9c55,	0x950c0a0b},
		{132,	0x95002cd0,	0x95044712,	0x950b9c55,	0x950c0a23},
		{136,	0x95002cd0,	0x9504471a,	0x950b9e55,	0x950c0a13},
    
        // 802.11 UNII
        {140,	0x95002cd0,	0x95044722,	0x950b9e55,	0x950c0a03},
		{149,	0x95002cd0,	0x9504472e,	0x950ba255,	0x950c0a1b},
		{153,	0x95002cd0,	0x95044736,	0x950ba255,	0x950c0a0b},
		{157,	0x95002cd4,	0x9504490a,	0x950ba255,	0x950c0a17},
		{161,	0x95002cd4,	0x95044912,	0x950ba255,	0x950c0a17},
		{165,	0x95002cd4,	0x9504491a,	0x950ba255,	0x950c0a17},

        //MMAC(Japan)J52 ch 34,38,42,46
		{34, 0x95002ccc, 0x9500499a, 0x9509be55, 0x950c0a0b},
		{38, 0x95002ccc, 0x9500499e, 0x9509be55, 0x950c0a13},
		{42, 0x95002ccc, 0x950049a2, 0x9509be55, 0x950c0a1b},
		{46, 0x95002ccc, 0x950049a6, 0x9509be55, 0x950c0a23},

};
UCHAR NUM_OF_5225_CHNL_1 = (sizeof(RF5225RegTable_1) / sizeof(RTMP_RF_REGS));

/*
    ==========================================================================
    Description:
        initialize the MLME task and its data structure (queue, spinlock, 
        timer, state machines).
        
    Return:
        always return NDIS_STATUS_SUCCESS
        
    ==========================================================================
*/
NDIS_STATUS MlmeInit(
    IN PRTMP_ADAPTER pAd) 
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DBGPRINT(RT_DEBUG_TRACE, "--> MLME Initialize\n");
    
    do 
    {
        Status = MlmeQueueInit(&pAd->Mlme.Queue);
        if(Status != NDIS_STATUS_SUCCESS) 
            break;

        pAd->Mlme.bRunning = FALSE;
        NdisAllocateSpinLock(&pAd->Mlme.TaskLock);
		NdisAllocateSpinLock(&pAd->Mlme.MemLock);   
        
        // initialize table
        BssTableInit(&pAd->ScanTab);
        
        // init state machines
        ASSERT(ASSOC_FUNC_SIZE == MAX_ASSOC_MSG * MAX_ASSOC_STATE);
        AssocStateMachineInit(pAd, &pAd->Mlme.AssocMachine, pAd->Mlme.AssocFunc);
        
        ASSERT(AUTH_FUNC_SIZE == MAX_AUTH_MSG * MAX_AUTH_STATE);
        AuthStateMachineInit(pAd, &pAd->Mlme.AuthMachine, pAd->Mlme.AuthFunc);
        
        ASSERT(AUTH_RSP_FUNC_SIZE == MAX_AUTH_RSP_MSG * MAX_AUTH_RSP_STATE);
        AuthRspStateMachineInit(pAd, &pAd->Mlme.AuthRspMachine, pAd->Mlme.AuthRspFunc);

        ASSERT(SYNC_FUNC_SIZE == MAX_SYNC_MSG * MAX_SYNC_STATE);
        SyncStateMachineInit(pAd, &pAd->Mlme.SyncMachine, pAd->Mlme.SyncFunc);

        ASSERT(WPA_PSK_FUNC_SIZE == MAX_WPA_PSK_MSG * MAX_WPA_PSK_STATE);
        WpaPskStateMachineInit(pAd, &pAd->Mlme.WpaPskMachine, pAd->Mlme.WpaPskFunc);

        // Since we are using switch/case to implement it, the init is different from the above 
        // state machine init
        MlmeCntlInit(pAd, &pAd->Mlme.CntlMachine, NULL);
        
        // Init mlme periodic timer
        RTMPInitTimer(pAd, &pAd->Mlme.PeriodicTimer, &MlmePeriodicExec);
		RTMPSetTimer(pAd, &pAd->Mlme.PeriodicTimer, MLME_TASK_EXEC_INTV);
    
		// Init timer to report link down event
		RTMPInitTimer(pAd, &pAd->Mlme.LinkDownTimer, &LinkDownExec);

		// Init rssi report
		pAd->Mlme.bTxRateReportPeriod = TRUE;
		RTMPInitTimer(pAd, &pAd->Mlme.RssiReportTimer, &MlmeRssiReportExec);

        // software-based RX Antenna diversity
        RTMPInitTimer(pAd, &pAd->RxAnt.RxAntDiversityTimer, &AsicRxAntEvalTimeout);



    } while (FALSE);

    DBGPRINT(RT_DEBUG_TRACE, "<-- MLME Initialize\n");

    return Status;
}

/*
    ==========================================================================
    Description:
        main loop of the MLME
    Pre:
        Mlme has to be initialized, and there are something inside the queue
    Note:
        This function is invoked from MPSetInformation and MPReceive;
        This task guarantee only one MlmeHandler will run. 

    ==========================================================================
 */
VOID MlmeHandler(
    IN PRTMP_ADAPTER pAd) 
{
    MLME_QUEUE_ELEM	*Elem = NULL;
    unsigned long	IrqFlags;

    // Only accept MLME and Frame from peer side, no other (control/data) frame should
    // get into this state machine
    RTMP_SEM_LOCK(&pAd->Mlme.TaskLock, IrqFlags);

    if(pAd->Mlme.bRunning) 
    {
		RTMP_SEM_UNLOCK(&pAd->Mlme.TaskLock, IrqFlags);
        return;
    } 
    else 
    {
        pAd->Mlme.bRunning = TRUE;
    }

	RTMP_SEM_UNLOCK(&pAd->Mlme.TaskLock, IrqFlags);

    while (!MlmeQueueEmpty(&pAd->Mlme.Queue)) 
    {
        //From message type, determine which state machine I should drive
        if (MlmeDequeue(&pAd->Mlme.Queue, &Elem)) 
        {
            // if dequeue success
            switch (Elem->Machine) 
            {
                case ASSOC_STATE_MACHINE:
                    StateMachinePerformAction(pAd, &pAd->Mlme.AssocMachine, Elem);
                    break;
                case AUTH_STATE_MACHINE:
                    StateMachinePerformAction(pAd, &pAd->Mlme.AuthMachine, Elem);
                    break;
                case AUTH_RSP_STATE_MACHINE:
                    StateMachinePerformAction(pAd, &pAd->Mlme.AuthRspMachine, Elem);
                    break;
                case SYNC_STATE_MACHINE:
                    StateMachinePerformAction(pAd, &pAd->Mlme.SyncMachine, Elem);
                    break;
                case MLME_CNTL_STATE_MACHINE:
                    MlmeCntlMachinePerformAction(pAd, &pAd->Mlme.CntlMachine, Elem);
                    break;    
				case WPA_PSK_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.WpaPskMachine, Elem);
					break;
                default:
                    DBGPRINT(RT_DEBUG_TRACE, "ERROR: Illegal machine in MlmeHandler()\n");
                    break;
            } // end of switch

            // free MLME element
            Elem->Occupied = FALSE;
            Elem->MsgLen = 0;
            
        }
        else {
            DBGPRINT_ERR("MlmeHandler: MlmeQueue empty\n");
        }
    }

    // Remove running state
	RTMP_SEM_LOCK(&pAd->Mlme.TaskLock, IrqFlags);
    pAd->Mlme.bRunning = FALSE;
	RTMP_SEM_UNLOCK(&pAd->Mlme.TaskLock, IrqFlags);
}

/*
    ==========================================================================
    Description:
        Destructor of MLME (Destroy queue, state machine, spin lock and timer)
    Parameters:
        Adapter - NIC Adapter pointer
    Post:
        The MLME task will no longer work properly
        
    ==========================================================================
 */
VOID MlmeHalt(
    IN PRTMP_ADAPTER pAd) 
{
    MLME_DISASSOC_REQ_STRUCT DisReq;
    MLME_QUEUE_ELEM          MsgElem;
	BOOLEAN					Cancelled;
 
    DBGPRINT(RT_DEBUG_TRACE, "==> MlmeHalt\n");

    // if connecting to an AP, try to DIS-ASSOC it before leave
    if (INFRA_ON(pAd) && !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
    {
        COPY_MAC_ADDR(DisReq.Addr, pAd->PortCfg.Bssid);
        DisReq.Reason =  REASON_DISASSOC_STA_LEAVING;

        MsgElem.Machine = ASSOC_STATE_MACHINE;
        MsgElem.MsgType = MT2_MLME_DISASSOC_REQ;
        MsgElem.MsgLen = sizeof(MLME_DISASSOC_REQ_STRUCT);
        NdisMoveMemory(MsgElem.Msg, &DisReq, sizeof(MLME_DISASSOC_REQ_STRUCT));

        MlmeDisassocReqAction(pAd, &MsgElem);

        RTMPusecDelay(100000);  // leave enough time (100 msec) for this DISASSOC frame
    }

    if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
	{
		// Set LED
		RTMPSetLED(pAd, LED_HALT);

        // disable BEACON generation and other BEACON related hardware timers
        AsicDisableSync(pAd);
    }
    
    // Cancel pending timers
    RTMPCancelTimer(&pAd->MlmeAux.AssocTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.ReassocTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.DisassocTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.AuthTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.BeaconTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->Mlme.PeriodicTimer, &Cancelled);
    RTMPCancelTimer(&pAd->Mlme.LinkDownTimer, &Cancelled);
    RTMPCancelTimer(&pAd->RxAnt.RxAntDiversityTimer, &Cancelled);
	RTMPCancelTimer(&pAd->Mlme.RssiReportTimer, &Cancelled);


    RTMPusecDelay(500000);    // 0.5 sec to guarantee timer canceled
    
    MlmeQueueDestroy(&pAd->Mlme.Queue);
    NdisFreeSpinLock(&pAd->Mlme.TaskLock);
    NdisFreeSpinLock(&pAd->Mlme.MemLock);

	DBGPRINT(RT_DEBUG_TRACE, "<== MlmeHalt\n");
}

/*
    ==========================================================================
    Description:
        This routine is executed periodically to -
        1. Decide if it's a right time to turn on PwrMgmt bit of all 
           outgoiing frames
        2. Calculate ChannelQuality based on statistics of the last
           period, so that TX rate won't toggling very frequently between a 
           successful TX and a failed TX.
        3. If the calculated ChannelQuality indicated current connection not 
           healthy, then a ROAMing attempt is tried here.
        
    ==========================================================================
 */

#define ADHOC_BEACON_LOST_TIME      (10*HZ)  // 4 sec 
VOID MlmePeriodicExec(
    IN	unsigned long data)
{
    RTMP_ADAPTER	*pAd = (RTMP_ADAPTER *)data;
    UCHAR BbpReg;

    pAd->Mlme.Now32 = jiffies;

#ifdef RALINK_ATE
	if(pAd->ate.Mode != ATE_STASTART)
	{
	    RTMPSetTimer(pAd, &pAd->Mlme.PeriodicTimer, MLME_TASK_EXEC_INTV);
	    return;
	}
#endif	// RALINK_ATE

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R17, &BbpReg);

	DBGPRINT(RT_DEBUG_TRACE,"Rx:%u AvgRssi1:%u AvgRssi2:%u Last8:%u Last8:%u R17:%x MCUfail:%u,defaultR17:%x:%x lastR17:%x u/l:%d Periodic:%u\n",pAd->debugdata[0],pAd->PortCfg.AvgRssi,pAd->PortCfg.AvgRssi2
	,pAd->PortCfg.AvgRssiX8,pAd->PortCfg.AvgRssi2X8,BbpReg,pAd->debugdata[1],pAd->BbpTuning.R17LowerBoundG,pAd->BbpTuning.R17UpperBoundG,pAd->BbpWriteLatch[17]
	,pAd->BbpTuning.R17LowerUpperSelect,pAd->Mlme.PeriodicRound);
	pAd->debugdata[0]=0;


#if 0
	if(BbpReg<0x20 | BbpReg > 0x40)
		printk("BBP R17 value error  %x\n",BbpReg);
#endif
	

    // Do nothing if the driver is starting halt state.
	// This might happen when timer already been fired before cancel timer with mlmehalt
	if ((RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) || 
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)) ||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_MEASUREMENT)) ||		
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)))
		return;

    // add the most up-to-date h/w raw counters into software variable, so that
    // the dynamic tuning mechanism below are based on most up-to-date information
    NICUpdateRawCounters(pAd);

    // if MGMT RING is full more than twice within 1 second, we consider there's
    // a hardware problem stucking the TX path. In this case, try a hardware reset
    // to recover the system
	if (pAd->RalinkCounters.MgmtRingFullCount >= 2)
		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR);
	else
		pAd->RalinkCounters.MgmtRingFullCount = 0;

    // danamic tune BBP R17 to find a balance between sensibility and noise isolation
    AsicBbpTuning(pAd);

    STAMlmePeriodicExec(pAd);

    pAd->RalinkCounters.LastOneSecRxOkDataCnt = pAd->RalinkCounters.OneSecRxOkDataCnt;
    // clear all OneSecxxx counters.
    pAd->RalinkCounters.OneSecBeaconSentCnt = 0;
    pAd->RalinkCounters.OneSecFalseCCACnt = 0;
    pAd->RalinkCounters.OneSecRxFcsErrCnt = 0;
    pAd->RalinkCounters.OneSecRxOkCnt = 0;
    pAd->RalinkCounters.OneSecTxFailCount = 0;
    pAd->RalinkCounters.OneSecTxNoRetryOkCount = 0;
    pAd->RalinkCounters.OneSecTxRetryOkCount = 0;
    pAd->RalinkCounters.OneSecRxOkDataCnt = 0;
    
    // TODO: for debug only. to be removed
    pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BE] = 0;
    pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BK] = 0;
    pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VI] = 0;
    pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VO] = 0;
    pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_BE] = 0;
    pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_BK] = 0;
    pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_VI] = 0;
    pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_VO] = 0;
    pAd->RalinkCounters.OneSecTxDoneCount = 0;
    pAd->RalinkCounters.OneSecTxAggregationCount = 0;
    pAd->RalinkCounters.OneSecRxAggregationCount = 0;
    
    pAd->Mlme.PeriodicRound ++;
	MlmeHandler(pAd);
    
    RTMPSetTimer(pAd, &pAd->Mlme.PeriodicTimer, MLME_TASK_EXEC_INTV+(pAd->Mlme.PeriodicRound%100));
}

VOID STAMlmePeriodicExec(
    PRTMP_ADAPTER pAd)
{
    ULONG			TxTotalCnt;
	TXRX_CSR4_STRUC CurTxRxCsr4;
	SHORT   		dbm;

    
#if WPA_SUPPLICANT_SUPPORT
    union iwreq_data wrqu;
#endif
	
    // WPA MIC error should block association attempt for 60 seconds
    if (pAd->PortCfg.bBlockAssoc && (pAd->PortCfg.LastMicErrorTime + (60 * HZ) < pAd->Mlme.Now32))
        pAd->PortCfg.bBlockAssoc = FALSE;
  

    DBGPRINT(RT_DEBUG_INFO,"MMCHK - PortCfg.Ssid[%d]=%c%c%c%c... MlmeAux.Ssid[%d]=%c%c%c%c...\n",
            pAd->PortCfg.SsidLen, pAd->PortCfg.Ssid[0], pAd->PortCfg.Ssid[1], pAd->PortCfg.Ssid[2], pAd->PortCfg.Ssid[3],
            pAd->MlmeAux.SsidLen, pAd->MlmeAux.Ssid[0], pAd->MlmeAux.Ssid[1], pAd->MlmeAux.Ssid[2], pAd->MlmeAux.Ssid[3]);

                    
    // danamic tune BBP R17 to find a balance between sensibility and noise isolation
    // AsicBbpTuning(pAd);
                
    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)) 
    {
        // update channel quality for Roaming and UI LinkQuality display
        MlmeCalculateChannelQuality(pAd, pAd->Mlme.Now32);
    
        // perform dynamic tx rate switching based on past TX history
        MlmeDynamicTxRateSwitching(pAd);
    }
    
    // must be AFTER MlmeDynamicTxRateSwitching() because it needs to know if
    // Radio is currently in noisy environment
    // Ralink suggeset to mark it.
    //AsicAdjustTxPower(pAd);
    
    // if Rx Antenna is DIVERSITY ON, then perform Software-based diversity evaluation
	if ((((pAd->RfIcType == RFIC_2529) && (pAd->Antenna.field.NumOfAntenna == 0) && (pAd->NicConfig2.field.Enable4AntDiversity == 1)) 
		|| ((pAd->Antenna.field.NumOfAntenna == 2) && (pAd->Antenna.field.TxDefaultAntenna == 0) && (pAd->Antenna.field.RxDefaultAntenna == 0)))
		&& (pAd->Mlme.PeriodicRound % 2 == 1))
	{
	    // check every 2 second. If rcv-beacon less than 5 in the past 2 second, then AvgRSSI is no longer a 
	    // valid indication of the distance between this AP and its clients.
	    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)) 
	    {
	    	SHORT	realavgrssi1, realavgrssi2;
	    	BOOLEAN	evaluate = FALSE;
	    	
	        if (pAd->PortCfg.NumOfAvgRssiSample < 5)
	        {
			    pAd->RxAnt.Pair1LastAvgRssi = (-115);
		    	pAd->RxAnt.Pair2LastAvgRssi = (-115);
	            DBGPRINT(RT_DEBUG_TRACE, "MlmePeriodicExec: no traffic/beacon, reset RSSI\n");
	        }
	        else
	            pAd->PortCfg.NumOfAvgRssiSample = 0;
        
			realavgrssi1 = (pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1PrimaryRxAnt] >> 3);

			DBGPRINT(RT_DEBUG_INFO,"Ant-realrssi0(%d),Lastrssi0(%d)\n",realavgrssi1, pAd->RxAnt.Pair1LastAvgRssi);
			
			if ((realavgrssi1 > (pAd->RxAnt.Pair1LastAvgRssi + 5)) || (realavgrssi1 < (pAd->RxAnt.Pair1LastAvgRssi - 5)))
			{
				evaluate = TRUE;
				pAd->RxAnt.Pair1LastAvgRssi = realavgrssi1;
			}
			if (pAd->RfIcType == RFIC_2529)
			{
				realavgrssi2 = (pAd->RxAnt.Pair2AvgRssi[pAd->RxAnt.Pair2PrimaryRxAnt] >> 3);

				DBGPRINT(RT_DEBUG_INFO, "Ant-realrssi1(%d),Lastrssi1(%d)\n",realavgrssi2,pAd->RxAnt.Pair2LastAvgRssi);
				
				if ((realavgrssi1 > (pAd->RxAnt.Pair2LastAvgRssi + 5)) || (realavgrssi2 < (pAd->RxAnt.Pair2LastAvgRssi - 5)))
				{
					evaluate = TRUE;
					pAd->RxAnt.Pair2LastAvgRssi = realavgrssi2;
				}
			}

			if (evaluate == TRUE)
			{
				AsicEvaluateSecondaryRxAnt(pAd);
			}
	    }
	    else
	    {
	    	UCHAR	temp;

	   		temp = pAd->RxAnt.Pair1PrimaryRxAnt;
	   		pAd->RxAnt.Pair1PrimaryRxAnt = pAd->RxAnt.Pair1SecondaryRxAnt;
	   		pAd->RxAnt.Pair1SecondaryRxAnt = temp;

	    	temp = pAd->RxAnt.Pair2PrimaryRxAnt;
	   		pAd->RxAnt.Pair2PrimaryRxAnt = pAd->RxAnt.Pair2SecondaryRxAnt;
	   		pAd->RxAnt.Pair2SecondaryRxAnt = temp;

	   		AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
	    }
	}

	// G band - set BBP_R62 to 0x02 when site survey or rssi<-82
	// A band - always set BBP_R62 to 0x04
	if ((pAd->Mlme.SyncMachine.CurrState == SYNC_IDLE) && (pAd->PortCfg.Channel <= 14))
	{
	    if (max(pAd->PortCfg.LastRssi, pAd->PortCfg.LastRssi2) >= (-82 + pAd->BbpRssiToDbmDelta))
	    {
	        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, 0x04);
	    }
	    else
	    {
	        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, 0x02);
	    }
	    DBGPRINT(RT_DEBUG_INFO, "STAMlmePeriodicExec - LastRssi=%d, BbpRssiToDbmDelta=%d\n", pAd->PortCfg.LastRssi, pAd->BbpRssiToDbmDelta);
	}
    
    if (INFRA_ON(pAd))
    {
        // Is PSM bit consistent with user power management policy?
        // This is the only place that will set PSM bit ON.
        MlmeCheckPsmChange(pAd, pAd->Mlme.Now32);
    
#if 0
        // patch Gemtek Prism2.5 AP bug, which stop sending BEACON for no reason
        if (INFRA_ON(pAd) &&
        	//2007/12/04:Modified by KH from 2500 to 2500*HZ/1000 
            (pAd->PortCfg.LastBeaconRxTime + 2500*HZ/1000 < pAd->Mlme.Now32))  // BEACON almost starving?
        {
            DBGPRINT(RT_DEBUG_TRACE, "!!! BEACON lost > 2.5 sec !!! Send ProbeRequest\n"); 
            EnqueueProbeRequest(pAd);
        }
#endif

		TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

		pAd->RalinkCounters.LastOneSecTotalTxCount = TxTotalCnt;
		//
		// Lost Beacon for almost one sec && no data traffic then set R17 to lowbound.
		//
		if (INFRA_ON(pAd) &&
			//2007/12/04:Modified by KH from 1000 to 1*HZ
			(pAd->PortCfg.LastBeaconRxTime + 1*HZ < pAd->Mlme.Now32) &&
			((TxTotalCnt + pAd->RalinkCounters.OneSecRxOkCnt < 600)))
		{
			if (pAd->PortCfg.Channel <= 14)
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->BbpTuning.R17LowerBoundG);
			}
			else
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->BbpTuning.R17LowerBoundA);
			}
		}

        // Check for EAPOL frame sent after MIC countermeasures
        if (pAd->PortCfg.MicErrCnt >= 3)
        {
            MLME_DISASSOC_REQ_STRUCT    DisassocReq;
                
            // disassoc from current AP first
            DBGPRINT(RT_DEBUG_TRACE, "MLME - disassociate with current AP after sending second continuous EAPOL frame\n");
            DisassocParmFill(pAd, &DisassocReq, pAd->PortCfg.Bssid, REASON_MIC_FAILURE);
            MlmeEnqueue(pAd, ASSOC_STATE_MACHINE, MT2_MLME_DISASSOC_REQ, 
                        sizeof(MLME_DISASSOC_REQ_STRUCT), &DisassocReq);
    
            pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_DISASSOC;
            pAd->PortCfg.bBlockAssoc = TRUE;
        }
            
        else 
        {
            // send out a NULL frame every 10 sec. for what??? inform "PwrMgmt" bit?
            if ((pAd->PortCfg.bAPSDCapable == FALSE) && ((pAd->Mlme.PeriodicRound % 10) == 8))
                RTMPSendNullFrame(pAd, pAd->PortCfg.TxRate, FALSE);
        
            if (CQI_IS_DEAD(pAd->Mlme.ChannelQuality))
            {
                DBGPRINT(RT_DEBUG_TRACE, "MMCHK - No BEACON. Dead CQI. Auto Recovery attempt #%d\n", pAd->RalinkCounters.BadCQIAutoRecoveryCount);
    
#if WPA_SUPPLICANT_SUPPORT
            if (pAd->PortCfg.WPA_Supplicant == TRUE) {
               // send disassoc event to wpa_supplicant 
               memset(&wrqu, 0, sizeof(wrqu));
               wrqu.data.flags = RT_DISASSOC_EVENT_FLAG;
               wireless_send_event(pAd->net_dev, IWEVCUSTOM, &wrqu, NULL);
            }
#endif

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
				if (pAd->PortCfg.bNativeWpa == TRUE)  // add by johnli
					wext_notify_event_assoc(pAd, SIOCGIWAP, FALSE);
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

                // Lost AP, send disconnect & link down event
                pAd->LinkDownReason=LINK_DOWN_LOSE_BEACON;
                LinkDown(pAd, FALSE);
		  
    
                // RTMPPatchMacBbpBug(pAd);
                MlmeAutoReconnectLastSSID(pAd);
            }
		else if (CQI_IS_BAD(pAd->Mlme.ChannelQuality))
		{
			pAd->RalinkCounters.BadCQIAutoRecoveryCount ++;
			DBGPRINT(RT_DEBUG_TRACE, "MMCHK - Bad CQI. Auto Recovery attempt #%d\n", pAd->RalinkCounters.BadCQIAutoRecoveryCount);
			MlmeAutoReconnectLastSSID(pAd);
		}
    // TODO: temp removed
#if 0
		else if (CQI_IS_POOR(pAd->Mlme.ChannelQuality))
		{
			// perform aggresive roaming only when SECURITY OFF or WEP64/128;
			// WPA and WPA-PSK has no aggresive roaming because re-negotiation 
			// between 802.1x supplicant and authenticator/AAA server is required
			// but can't be guaranteed.
			if (pAd->PortCfg.AuthMode < Ndis802_11AuthModeWPA)
				MlmeCheckForRoaming(pAd, pAd->Mlme.Now32);
		}
#endif
		// Grace: Add auto seamless roaming
		if (pAd->PortCfg.bFastRoaming)
		{
			// Check the RSSI value, we should begin the roaming attempt
			INT RxSignal = pAd->PortCfg.LastRssi - pAd->BbpRssiToDbmDelta;

			DBGPRINT(RT_DEBUG_TRACE, "RxSignal %d\n", RxSignal); 
			// Only perform action when signal is less than or equal to setting from the UI or registry
			if (pAd->PortCfg.LastRssi <= (CHAR) (pAd->BbpRssiToDbmDelta - pAd->PortCfg.dBmToRoam))
			{
				MlmeCheckForFastRoaming(pAd, pAd->Mlme.Now32);
			}
		}
        }
    }
    else if (ADHOC_ON(pAd))
    {
#if 1    
        // 1. 2003-04-17 john. this is a patch that driver forces a BEACON out if ASIC fails
        // the "TX BEACON competition" for the entire past 1 sec.
        // So that even when ASIC's BEACONgen engine been blocked
        // by peer's BEACON due to slower system clock, this STA still can send out
        // minimum BEACON to tell the peer I'm alive.
        // drawback is that this BEACON won't be well aligned at TBTT boundary.
        // 2. avoid mlme-queue full while doing radar detection
	if ((pAd->PortCfg.bIEEE80211H == 0) || (pAd->PortCfg.RadarDetect.RDMode == RD_NORMAL_MODE)) 
        if (pAd->RalinkCounters.OneSecBeaconSentCnt <= 0)  
            EnqueueBeaconFrame(pAd);              // software send BEACON
#endif
        // if all 11b peers leave this BSS more than 5 seconds, update Tx rate,
        // restore outgoing BEACON to support B/G-mixed mode
        if ((pAd->PortCfg.Channel <= 14)             &&
            (pAd->PortCfg.MaxTxRate <= RATE_11)      &&
            (pAd->PortCfg.MaxDesiredRate > RATE_11)  &&
            //2007/12/04:Modified by KH from 5000 to 5*HZ
            ((pAd->PortCfg.Last11bBeaconRxTime + 5*HZ) < pAd->Mlme.Now32))
        {
            DBGPRINT(RT_DEBUG_TRACE, "MMCHK - last 11B peer left, update Tx rates\n"); 

            NdisMoveMemory(pAd->ActiveCfg.SupRate, pAd->PortCfg.SupRate, MAX_LEN_OF_SUPPORTED_RATES);
            pAd->ActiveCfg.SupRateLen = pAd->PortCfg.SupRateLen;

            MlmeUpdateTxRates(pAd, FALSE);
            MakeIbssBeacon(pAd);        // re-build BEACON frame 
            AsicEnableIbssSync(pAd);    // copy to on-chip memory
        }
        
        //radar detect
		if (((pAd->PortCfg.PhyMode == PHY_11A) || (pAd->PortCfg.PhyMode == PHY_11ABG_MIXED)) && (pAd->PortCfg.bIEEE80211H == 1) && RadarChannelCheck(pAd, pAd->PortCfg.Channel))
		{
			// need to check channel availability, after switch channel
			if (pAd->PortCfg.RadarDetect.RDMode == RD_SILENCE_MODE)
			{
				pAd->PortCfg.RadarDetect.RDCount++;
				
				// channel availability check time is 60sec
				if (pAd->PortCfg.RadarDetect.RDCount > 65)
				{
					if (RadarDetectionStop(pAd))
					{
						pAd->ExtraInfo = DETECT_RADAR_SIGNAL;
						pAd->PortCfg.RadarDetect.RDCount = 0;		// stat at silence mode and detect radar signal
						DBGPRINT(RT_DEBUG_TRACE, "Found radar signal!!!\n\n");
					}
					else
					{
						DBGPRINT(RT_DEBUG_TRACE, "Not found radar signal, start send beacon\n");
						AsicEnableIbssSync(pAd);
						pAd->PortCfg.RadarDetect.RDMode = RD_NORMAL_MODE;
					}
				}
			}
		}
        
#ifndef SINGLE_ADHOC_LINKUP  
        // If all peers leave, and this STA becomes the last one in this IBSS, then change MediaState
        // to DISCONNECTED. But still holding this IBSS (i.e. sending BEACON) so that other STAs can
        // join later.
        if ((pAd->PortCfg.LastBeaconRxTime + ADHOC_BEACON_LOST_TIME < pAd->Mlme.Now32) &&
            OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
        {
            DBGPRINT(RT_DEBUG_TRACE, "MMCHK - excessive BEACON lost, last STA in this IBSS, MediaState=Disconnected\n"); 
    
            OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);              
               
            // clean up previous SCAN result, add current BSS back to table if any
            BssTableDeleteEntry(&pAd->ScanTab, pAd->PortCfg.Bssid, pAd->PortCfg.Channel);
    
            pAd->PortCfg.LastScanTime = pAd->Mlme.Now32;
        }      
#endif
    }
    else // no INFRA nor ADHOC connection
    {
        DBGPRINT(RT_DEBUG_INFO, "MLME periodic exec, no association so far\n");
        if ((pAd->PortCfg.bAutoReconnect == TRUE) && 
            (MlmeValidateSSID(pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen) == TRUE))
        {
            if ((pAd->ScanTab.BssNr==0) && (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE))
            {
                MLME_SCAN_REQ_STRUCT       ScanReq;
              //2007/12/04:Modified by KH from 10*1000 to 10*HZ
                if ((pAd->PortCfg.LastScanTime + 10 * HZ) < pAd->Mlme.Now32)
                {
                    DBGPRINT(RT_DEBUG_TRACE, "CNTL - No matching BSS, start a new ACTIVE scan SSID[%s]\n", pAd->MlmeAux.AutoReconnectSsid);
                    ScanParmFill(pAd, &ScanReq, pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen, BSS_ANY, SCAN_ACTIVE);
                    MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ, sizeof(MLME_SCAN_REQ_STRUCT), &ScanReq);
                    pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;
                    // Reset Missed scan number
                    pAd->PortCfg.LastScanTime = pAd->Mlme.Now32;
                }
                else if (pAd->PortCfg.BssType == BSS_ADHOC)  // Quit the forever scan when in a very clean room
                {
                    // set wpapsk key in case of wpanone
                    if ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPANone)&&
                        ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) ||
                         (pAd->PortCfg.WepStatus == Ndis802_11Encryption3Enabled)) &&
                        (pAd->PortCfg.WpaState == SS_START))
                    {
                        RTMPWPANoneAddKeyProc(pAd, pAd->PortCfg.PskKey.Key);
                        // turn on the flag of PortCfg.WpaState as reading profile 
                        // and reset after adding key
                        pAd->PortCfg.WpaState = SS_NOTUSE;
                    }                     

                    MlmeAutoReconnectLastSSID(pAd);
                }    
            }           
            else if (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE)
            {   
                if ((pAd->Mlme.PeriodicRound % pAd->scanTabCleanTime) == 0)
                    {
                        MlmeAutoScan(pAd);
                        pAd->PortCfg.LastScanTime = pAd->Mlme.Now32;
                    }
                else
                {    
                    // set wpapsk key in case of wpanone
                    if ((pAd->PortCfg.BssType == BSS_ADHOC) &&
                        (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPANone)&&
                        ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) ||
                         (pAd->PortCfg.WepStatus == Ndis802_11Encryption3Enabled)) &&
                        (pAd->PortCfg.WpaState == SS_START))
                    {
                        RTMPWPANoneAddKeyProc(pAd, pAd->PortCfg.PskKey.Key);
                        // turn on the flag of PortCfg.WpaState as reading profile 
                        // and reset after adding key
                        pAd->PortCfg.WpaState = SS_NOTUSE;
                    }                     

                    MlmeAutoReconnectLastSSID(pAd);
                }               
                DBGPRINT(RT_DEBUG_INFO, "pAd->PortCfg.bAutoReconnect is TRUE\n");
            }
        }
    }

    if (((pAd->Mlme.PeriodicRound % 2) == 0) &&
		(INFRA_ON(pAd) || ADHOC_ON(pAd)))
	{
		RTMPSetSignalLED(pAd, pAd->PortCfg.LastRssi - pAd->BbpRssiToDbmDelta);
	}


	if (((pAd->Mlme.PeriodicRound % 2) == 0) &&
		(INFRA_ON(pAd) || ADHOC_ON(pAd)))
	{
		RTMPSetSignalLED(pAd, pAd->PortCfg.LastRssi - pAd->BbpRssiToDbmDelta);

		if ((pAd->RfIcType == RFIC_5325) || (pAd->RfIcType == RFIC_2529))
		{
			pAd->Mlme.bTxRateReportPeriod = TRUE;
			RTMP_IO_WRITE32(pAd, TXRX_CSR1, 0x9eaa9eaf);
			//Start a timer, if no data come in then switch to Rssi-A/Rssi-B.
			RTMPSetTimer(pAd, &pAd->Mlme.RssiReportTimer, 300);
			DBGPRINT(RT_DEBUG_INFO, "Collect Rssi-A/TxRate\n");
		}
	}	

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		//
		// Modify retry times (maximum 15) on low data traffic.
		// Should fix ping lost.
		//
		dbm = pAd->PortCfg.AvgRssi - pAd->BbpRssiToDbmDelta;
		if ((pAd->RfIcType == RFIC_5325) || (pAd->RfIcType == RFIC_2529))
		{
			if (pAd->PortCfg.AvgRssi2 > pAd->PortCfg.AvgRssi)
			{
				dbm = pAd->PortCfg.AvgRssi2 - pAd->BbpRssiToDbmDelta;
			}
		}

		//
		// Only on infrastructure mode will change the RetryLimit.
		//
		if (INFRA_ON(pAd))
		{
			if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MAX_RETRY_ENABLED))
			{
				if (pAd->RalinkCounters.OneSecTxNoRetryOkCount > 15)
				{
					RTMP_IO_READ32(pAd, TXRX_CSR4, &CurTxRxCsr4.word);
					CurTxRxCsr4.field.ShortRetryLimit = 0x07;
					CurTxRxCsr4.field.LongRetryLimit = 0x04;
					RTMP_IO_WRITE32(pAd, TXRX_CSR4, CurTxRxCsr4.word);
					OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MAX_RETRY_ENABLED);
				}
			}
			else
			{
				if (pAd->RalinkCounters.OneSecTxNoRetryOkCount <= 15)
				{
					RTMP_IO_READ32(pAd, TXRX_CSR4, &CurTxRxCsr4.word);
					CurTxRxCsr4.field.ShortRetryLimit = 0x0f;
					CurTxRxCsr4.field.LongRetryLimit = 0x0f;
					RTMP_IO_WRITE32(pAd, TXRX_CSR4, CurTxRxCsr4.word);
					OPSTATUS_SET_FLAG(pAd, fOP_STATUS_MAX_RETRY_ENABLED);
				}
			}
		}

		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_RTS_PROTECTION_ENABLE))
		{
			if ((dbm > -60) || (pAd->RalinkCounters.OneSecTxNoRetryOkCount > 15))
				OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_RTS_PROTECTION_ENABLE);			
		}
		else 
		{
			//
			// for long distance case, turn on RTS to protect data frame.
			//
			if ((dbm <= -60) && (pAd->RalinkCounters.OneSecTxNoRetryOkCount <= 15))
				OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RTS_PROTECTION_ENABLE);
		}
	}


}    
    
VOID LinkDownExec(
    IN	unsigned long data) 
{
    //RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)data;
}

VOID MlmeAutoScan(
    IN PRTMP_ADAPTER pAd)
{
    // check CntlMachine.CurrState to avoid collision with NDIS SetOID request
    
    if (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE)
    {
        DBGPRINT(RT_DEBUG_TRACE, "MMCHK === Driver clean scan tab and auto scan ===\n");
        MlmeEnqueue(pAd, 
                    MLME_CNTL_STATE_MACHINE, 
                    OID_802_11_BSSID_LIST_SCAN, 
                    0, 
                    NULL);
        MlmeHandler(pAd);
    }
}
	
VOID MlmeAutoRecoverNetwork(
    IN PRTMP_ADAPTER pAd)
{
    // check CntlMachine.CurrState to avoid collision with NDIS SetOID request
    if (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE)
    {
        NDIS_802_11_SSID OidSsid;
        OidSsid.SsidLength = pAd->PortCfg.SsidLen;
        NdisMoveMemory(OidSsid.Ssid, pAd->PortCfg.Ssid, pAd->PortCfg.SsidLen);

        DBGPRINT(RT_DEBUG_TRACE, "MMCHK - Driver auto recovering network - %s\n", pAd->PortCfg.Ssid);

                    
        MlmeEnqueue(pAd, 
                    MLME_CNTL_STATE_MACHINE, 
                    OID_802_11_SSID, 
                    sizeof(NDIS_802_11_SSID), 
                    &OidSsid);
        MlmeHandler(pAd);
    }

}

//2007/11/09:KH modified the original MlmeAutoReconnecLastSSID. Because our original reconnection function sometimes uses the out-of-date
//scantable, add to function to check if the scantalbe is too old to use. If not, do autoconnection or do scan-channel in advance.
VOID MlmeAutoReconnectLastSSID(
    IN PRTMP_ADAPTER pAd)
{
    // check CntlMachine.CurrState to avoid collision with NDIS SetOID request
    MLME_SCAN_REQ_STRUCT       ScanReq;
    ULONG Now= pAd->Mlme.Now32;
     //2007/12/04:Modified by KH from 10*1000 to 10*HZ
    if ((pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE) && ((pAd->PortCfg.LastScanTime + 10 * HZ) >= Now))
    {
	    if (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE &&
	        (MlmeValidateSSID(pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen) == TRUE))
	    {
	        NDIS_802_11_SSID OidSsid;
	        OidSsid.SsidLength = pAd->MlmeAux.AutoReconnectSsidLen;
	        NdisMoveMemory(OidSsid.Ssid, pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen);

	        DBGPRINT(RT_DEBUG_TRACE, "Driver auto reconnect to last OID_802_11_SSID setting - (%s)\n", pAd->MlmeAux.AutoReconnectSsid);

	     	/* carella modify: if no SSID, sta should not try to connect AP in scan table */
			if(OidSsid.SsidLength)
			{
	        	MlmeEnqueue(pAd, 
	                    MLME_CNTL_STATE_MACHINE, 
	                    OID_802_11_SSID, 
	                    sizeof(NDIS_802_11_SSID), 
	                    &OidSsid);
	        	MlmeHandler(pAd);
			}
	    }
    }
	else
	 //2007/12/04:Modified by KH from 10*1000 to 10*HZ
		if ((pAd->PortCfg.LastScanTime + 10 * HZ) < Now)
        {
            // check CntlMachine.CurrState to avoid collision with NDIS SetOID request
            DBGPRINT(RT_DEBUG_TRACE, "MMCHK - AutoReconnect, No eligable entry, try new scan!\n");
            pAd->PortCfg.ScanCnt = 2;
            pAd->PortCfg.LastScanTime = Now;
            ScanParmFill(pAd, &ScanReq, pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen, BSS_ANY, SCAN_ACTIVE);
            MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ, sizeof(MLME_SCAN_REQ_STRUCT), &ScanReq);
            pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;
        }
}

/*
    ==========================================================================
    Validate SSID for connection try and rescan purpose
    Valid SSID will have visible chars only.
    The valid length is from 0 to 32.
    ==========================================================================
 */
BOOLEAN	MlmeValidateSSID(
    IN PUCHAR   pSsid,
    IN UCHAR    SsidLen)
{
	int	index;

    if (SsidLen > MAX_LEN_OF_SSID)
		return (FALSE);

	// Check each character value
	for (index = 0; index < SsidLen; index++)
	{
		if (pSsid[index] < 0x20)
			return (FALSE);
	}

	// All checked
	return (TRUE);
}

/*
    ==========================================================================
    Description:
        This routine checks if there're other APs out there capable for
        roaming. Caller should call this routine only when Link up in INFRA mode
        and channel quality is below CQI_GOOD_THRESHOLD.
        
    Output:
    ==========================================================================
 */
VOID MlmeCheckForRoaming(
    IN PRTMP_ADAPTER pAd,
    IN ULONG    Now32)
{
    USHORT     i;
    BSS_TABLE  *pRoamTab = &pAd->MlmeAux.RoamTab;
    BSS_ENTRY  *pBss;

	DBGPRINT(RT_DEBUG_TRACE, "==> MlmeCheckForRoaming::pAd->ScanTab.BssNr = %d\n", pAd->ScanTab.BssNr);

	// put all roaming candidates into RoamTab, and sort in RSSI order
	BssTableInit(pRoamTab);
	
    for (i = 0; i < pAd->ScanTab.BssNr; i++)
    {
        pBss = &pAd->ScanTab.BssEntry[i];
 	 DBGPRINT(RT_DEBUG_TRACE, "MlmeCheckForRoaming::pBss->LastBeaconRxTime = %ld\n", pBss->LastBeaconRxTime);       

	if ((pBss->LastBeaconRxTime + BEACON_LOST_TIME) < Now32)
	{
		DBGPRINT(RT_DEBUG_TRACE, "1: AP disappear::Now32 = %d\n", Now32);
		continue;    // AP disappear
        }
	if (pBss->Rssi <= RSSI_THRESHOLD_FOR_ROAMING)
       {    
		DBGPRINT(RT_DEBUG_TRACE, "2: RSSI too weak::Rssi[%d] - RSSI_THRESHOLD_FOR_ROAMING[%d]\n", pBss->Rssi, RSSI_THRESHOLD_FOR_ROAMING);
		continue;    // RSSI too weak. forget it.
       }
	if (MAC_ADDR_EQUAL(pBss->Bssid, pAd->PortCfg.Bssid))
       {
		DBGPRINT(RT_DEBUG_TRACE, "3: skip current AP\n");
		continue;    // skip current AP
       }
	if (!SSID_EQUAL(pBss->Ssid, pBss->SsidLen, pAd->PortCfg.Ssid, pAd->PortCfg.SsidLen))
	{
		DBGPRINT(RT_DEBUG_TRACE, "4: skip different SSID\n");
		continue;    // skip different SSID
	}
	if (pBss->Rssi < (pAd->PortCfg.LastRssi + RSSI_DELTA))
       {    
		DBGPRINT(RT_DEBUG_TRACE, "5: only AP with stronger RSSI is eligible for roaming\n");
		continue;    // only AP with stronger RSSI is eligible for roaming
	}
       // AP passing all above rules is put into roaming candidate table        
        NdisMoveMemory(&pRoamTab->BssEntry[pRoamTab->BssNr], pBss, sizeof(BSS_ENTRY));
        pRoamTab->BssNr += 1;
    }

    if (pRoamTab->BssNr > 0)
    {
        // check CntlMachine.CurrState to avoid collision with NDIS SetOID request
        if (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE)
        {
          	pAd->RalinkCounters.PoorCQIRoamingCount ++;
            DBGPRINT(RT_DEBUG_TRACE, "MMCHK - Roaming attempt #%d\n", pAd->RalinkCounters.PoorCQIRoamingCount);
            MlmeEnqueue(pAd, MLME_CNTL_STATE_MACHINE, MT2_MLME_ROAMING_REQ, 0, NULL);
            MlmeHandler(pAd);
        }
    }
	DBGPRINT(RT_DEBUG_TRACE, "<== MlmeCheckForRoaming(# of candidate= %d)\n",pRoamTab->BssNr);   
}

/*
	==========================================================================
	Description:
		This routine checks if there're other APs out there capable for
		roaming. Caller should call this routine only when link up in INFRA mode
		and channel quality is below CQI_GOOD_THRESHOLD.

	IRQL = DISPATCH_LEVEL

	Output:
	==========================================================================
 */
VOID MlmeCheckForFastRoaming(
	IN	PRTMP_ADAPTER	pAd,
	IN	unsigned long   Now)
{
	USHORT		i;
	BSS_TABLE	*pRoamTab = &pAd->MlmeAux.RoamTab;
	BSS_ENTRY	*pBss;
	MLME_SCAN_REQ_STRUCT       ScanReq;	
	DBGPRINT(RT_DEBUG_TRACE, "==> MlmeCheckForFastRoaming\n");
	//
    // If Scan talbe is fresh within 10 sec, then we can select the AP with the best RSSI
    // Otherwise refresh this scan table.
    //
 //2007/12/04:Modified by KH from 10*1000 to 10*HZ
    if ((pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE) && ((pAd->PortCfg.LastScanTime + 10 * HZ) >= Now))
    {
            BssTableInit(pRoamTab);
	    // put all roaming candidates into RoamTab, and sort in RSSI order
	
	    for (i = 0; i < pAd->ScanTab.BssNr; i++)
	    {
		    pBss = &pAd->ScanTab.BssEntry[i];

		    //if ((pBss->Rssi <= 45) && (pBss->Channel == pAd->PortCfg.Channel))
		    /*2007/11/09:KH replaced the original line 
			"if ((pBss->Rssi <= 45) && (pBss->Channel == pAd->PortCfg.Channel))"
			by if (pBss->Rssi <= 45) to allow the roaming could work in different channels.
			*/
		    if (pBss->Rssi <= 45)
			    continue;    // RSSI too weak. forget it.
		    if (MAC_ADDR_EQUAL(pBss->Bssid, pAd->PortCfg.Bssid))
			    continue;    // skip current AP

		    DBGPRINT(RT_DEBUG_TRACE,"MlmeRoaming Check SSID - PortCfg.Ssid[%d]=%s... MlmeAux.Ssid[%d]=%s\n",
											            pAd->PortCfg.SsidLen, pAd->PortCfg.Ssid,
											            pAd->MlmeAux.SsidLen, pAd->MlmeAux.Ssid);
                    //2007/12/06:KH add to fix roaming failed with hidden SSID
		    if (!SSID_EQUAL(pBss->Ssid, pBss->SsidLen, pAd->PortCfg.Ssid, pAd->PortCfg.SsidLen) &&pBss->SsidLen>0&&pBss->Ssid[0]!=0 )
				continue;    // skip different SSID
		    if (pBss->Rssi < (pAd->PortCfg.LastRssi + RSSI_DELTA)) 
				continue;    // skip AP without better RSSI
		
		    DBGPRINT(RT_DEBUG_TRACE, "LastRssi = %d, pBss->Rssi = %d\n", pAd->PortCfg.LastRssi, pBss->Rssi);
			// AP passing all above rules is put into roaming candidate table        
		    DBGPRINT(RT_DEBUG_TRACE, "AP passing all above rules is put into roaming candidate table and BssrNR=%d\n",pRoamTab->BssNr);      
		    NdisMoveMemory(&pRoamTab->BssEntry[pRoamTab->BssNr], pBss, sizeof(BSS_ENTRY));
		  
                    //2007/12/06:KH add to fix roaming failed with hidden SSID
		    if(pBss->SsidLen==0||pBss->Ssid[0]==0)
		    {
			DBGPRINT(RT_DEBUG_TRACE, "Roaming With NULL SSID and BSSID[#%d]=%s", pRoamTab->BssNr,pBss->Bssid);
			pRoamTab->BssEntry[pRoamTab->BssNr].SsidLen=pAd->PortCfg.SsidLen;
			NdisMoveMemory(pRoamTab->BssEntry[pRoamTab->BssNr].Ssid, pAd->PortCfg.Ssid, pAd->PortCfg.SsidLen);
			DBGPRINT(RT_DEBUG_TRACE, "Roaming With NULL SSID after modified the BSSID[#%d]=%s with SSID[%d]=%s", pRoamTab->BssNr,pBss->Bssid,pRoamTab->BssEntry[pRoamTab->BssNr].SsidLen,pRoamTab->BssEntry[pRoamTab->BssNr].Ssid);
		     }
		     pRoamTab->BssNr += 1;
		}

		if (pRoamTab->BssNr > 0)
		{
		
			pAd->RalinkCounters.PoorCQIRoamingCount ++;
			DBGPRINT(RT_DEBUG_TRACE, "MMCHK - Roaming attempt #%d\n", pAd->RalinkCounters.PoorCQIRoamingCount);
			MlmeEnqueue(pAd, MLME_CNTL_STATE_MACHINE, MT2_MLME_ROAMING_REQ, 0, NULL);
			MlmeHandler(pAd);
		
		}
        }
	// Maybe site survey required
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, "Roaming:Do site_survey\n");
		//correct ISSUE 13
                if ((pAd->PortCfg.LastScanTime + (10*HZ)) < Now)							// BUG 10 sec
		{
			
			// check CntlMachine.CurrState to avoid collision with NDIS SetOID request
			DBGPRINT(RT_DEBUG_TRACE, "MMCHK - Roaming, No eligable entry, try new scan!\n");
			pAd->PortCfg.ScanCnt = 2;
			pAd->PortCfg.LastScanTime = Now;
			//2007/11/09:KH replaced the orignal scan type (SCAN_ACTIVE) by FAST_SCAN_ACTIVE to decrease the scan time
			ScanParmFill(pAd, &ScanReq, pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen, BSS_ANY, FAST_SCAN_ACTIVE);
   			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ, 
                          sizeof(MLME_SCAN_REQ_STRUCT), &ScanReq);
                     pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;
		}
	}

	DBGPRINT(RT_DEBUG_TRACE, "<== MlmeCheckForFastRoaming\n");	
}


/*
    ==========================================================================
    Description:
        This routine calculates TxPER, RxPER of the past N-sec period. And 
        according to the calculation result, ChannelQuality is calculated here 
        to decide if current AP is still doing the job. 

        If ChannelQuality is not good, a ROAMing attempt may be tried later.
    Output:
        PortCfg.ChannelQuality - 0..100

        
    NOTE: This routine decide channle quality based on RX CRC error ratio.
        Caller should make sure a function call to NICUpdateRawCounters(pAd)
        is performed right before this routine, so that this routine can decide
        channel quality based on the most up-to-date information
    ==========================================================================
 */
VOID MlmeCalculateChannelQuality(
    IN PRTMP_ADAPTER pAd,
    IN ULONG Now32)
{
    ULONG TxOkCnt, TxCnt, TxPER, TxPRR;
    ULONG RxCnt, RxPER;
	UCHAR NorRssi;

    //
    // calculate TX packet error ratio and TX retry ratio - if too few TX samples, skip TX related statistics
    //
    TxOkCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + pAd->RalinkCounters.OneSecTxRetryOkCount;
    TxCnt = TxOkCnt + pAd->RalinkCounters.OneSecTxFailCount;
    if (TxCnt < 5) 
    {
        TxPER = 0;
        TxPRR = 0;
    }
    else 
    {
        TxPER = (pAd->RalinkCounters.OneSecTxFailCount * 100) / TxCnt; 
        TxPRR = ((TxCnt - pAd->RalinkCounters.OneSecTxNoRetryOkCount) * 100) / TxCnt;
    }

    //
    // calculate RX PER - don't take RxPER into consideration if too few sample
    //
    RxCnt = pAd->RalinkCounters.OneSecRxOkCnt + pAd->RalinkCounters.OneSecRxFcsErrCnt;
    if (RxCnt < 5)
        RxPER = 0;  
    else
        RxPER = (pAd->RalinkCounters.OneSecRxFcsErrCnt * 100) / RxCnt;
    
    //
    // decide ChannelQuality based on: 1)last BEACON received time, 2)last RSSI, 3)TxPER, and 4)RxPER
    //
    if (INFRA_ON(pAd) && 
        (TxOkCnt < 2) && // no heavy traffic
        (pAd->PortCfg.LastBeaconRxTime + BEACON_LOST_TIME < Now32))
    {
       	DBGPRINT(RT_DEBUG_TRACE, "BEACON lost > %d msec with TxOkCnt=%d -> CQI=0\n", BEACON_LOST_TIME*1000/HZ, TxOkCnt); 
       	pAd->Mlme.ChannelQuality = 0;
    }
    else
    {
		// Normalize Rssi
		if (pAd->PortCfg.LastRssi > 0x50)
			NorRssi = 100;
		else if (pAd->PortCfg.LastRssi < 0x20)
			NorRssi = 0;
		else
			NorRssi = (pAd->PortCfg.LastRssi - 0x20) * 2;
		
        // ChannelQuality = W1*RSSI + W2*TxPRR + W3*RxPER    (RSSI 0..100), (TxPER 100..0), (RxPER 100..0)
        pAd->Mlme.ChannelQuality = (RSSI_WEIGHTING * NorRssi + 
                                   TX_WEIGHTING * (100 - TxPRR) + 
                                   RX_WEIGHTING* (100 - RxPER)) / 100;
        if (pAd->Mlme.ChannelQuality >= 100)
            pAd->Mlme.ChannelQuality = 100;
    }
    
    DBGPRINT(RT_DEBUG_INFO, "MMCHK - CQI= %d (Tx Fail=%d/Retry=%d/Total=%d, Rx Fail=%d/Total=%d, RSSI=%d dbm)\n", 
        pAd->Mlme.ChannelQuality, 
        pAd->RalinkCounters.OneSecTxFailCount, 
        pAd->RalinkCounters.OneSecTxRetryOkCount, 
        TxCnt, 
        pAd->RalinkCounters.OneSecRxFcsErrCnt, 
        RxCnt, pAd->PortCfg.LastRssi - pAd->BbpRssiToDbmDelta);

}

/*
    ==========================================================================
    Description:
        This routine calculates the acumulated TxPER of eaxh TxRate. And 
        according to the calculation result, change PortCfg.TxRate which 
        is the stable TX Rate we expect the Radio situation could sustained. 

        PortCfg.TxRate will change dynamically within {RATE_1/RATE_6, MaxTxRate} 
    Output:
        PortCfg.TxRate - 
        

    NOTE:
        call this routine every second
    ==========================================================================
 */
VOID MlmeDynamicTxRateSwitching(
    IN PRTMP_ADAPTER pAd)
{
    UCHAR   UpRate, DownRate, CurrRate;
    ULONG   TxTotalCnt, NewBasicRateBitmap;
    ULONG   TxErrorRatio = 0;
    BOOLEAN fUpgradeQuality = FALSE;
    SHORT   dbm = pAd->PortCfg.AvgRssi - pAd->BbpRssiToDbmDelta;

	if ((pAd->RfIcType == RFIC_5325) || (pAd->RfIcType == RFIC_2529))
	{
		if (pAd->PortCfg.AvgRssi2 > pAd->PortCfg.AvgRssi)
		{
			dbm = pAd->PortCfg.AvgRssi2 - pAd->BbpRssiToDbmDelta;
		}
	}

    CurrRate = pAd->PortCfg.TxRate;

    // do not reply ACK using TX rate higher than normal DATA TX rate
    NewBasicRateBitmap = pAd->PortCfg.BasicRateBitmap & BasicRateMask[CurrRate];
    RTMP_IO_WRITE32(pAd, TXRX_CSR5, NewBasicRateBitmap);
    
    // if no traffic in the past 1-sec period, don't change TX rate,
    // but clear all bad history. because the bad history may affect the next 
    // Chariot throughput test
    TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
                 pAd->RalinkCounters.OneSecTxRetryOkCount + 
                 pAd->RalinkCounters.OneSecTxFailCount;

    if (TxTotalCnt)
        TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) *100) / TxTotalCnt;
    
    DBGPRINT_RAW(RT_DEBUG_TRACE,"%d: NDIS push BE=%d, BK=%d, VI=%d, VO=%d, TX/RX AGGR=<%d,%d>, p-NDIS=%d, RSSI=%d, ACKbmap=%03x, PER=%d%%\n",
        RateIdToMbps[CurrRate],
        pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BE],
        pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BK],
        pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VI],
        pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VO],
        pAd->RalinkCounters.OneSecTxAggregationCount,
        pAd->RalinkCounters.OneSecRxAggregationCount,
        pAd->RalinkCounters.PendingNdisPacketCount, 
        dbm,
        NewBasicRateBitmap & 0xfff,
        TxErrorRatio);
    
    if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
        return;
        
    //
    // CASE 1. when TX samples are fewer than 15, then decide TX rate solely on RSSI
    //         (criteria copied from RT2500 for Netopia case)
    //
    if (TxTotalCnt <= 15)
    {
        TxErrorRatio = 0;
        pAd->DrsCounters.TxRateUpPenalty = 0;
        NdisZeroMemory(pAd->DrsCounters.TxQuality, MAX_LEN_OF_SUPPORTED_RATES);
        NdisZeroMemory(pAd->DrsCounters.PER, MAX_LEN_OF_SUPPORTED_RATES);

        if (dbm >= -65)
            pAd->PortCfg.TxRate = RATE_54;
        else if (dbm >= -66)
            pAd->PortCfg.TxRate = RATE_48;
        else if (dbm >= -70)
            pAd->PortCfg.TxRate = RATE_36;
        else if (dbm >= -74)
            pAd->PortCfg.TxRate = RATE_24;
        else if (dbm >= -77)
            pAd->PortCfg.TxRate = RATE_18;  
        else if (dbm >= -79)
            pAd->PortCfg.TxRate = RATE_12;
        else if (dbm >= -81)
		{
			// in 11A or 11G-only mode, no CCK rates available
			if ((pAd->PortCfg.Channel > 14) || (pAd->PortCfg.PhyMode == PHY_11G))
				pAd->PortCfg.TxRate = RATE_9;
			else 
				pAd->PortCfg.TxRate = RATE_11;
		}		
        else 
        {
            // in 11A or 11G-only mode, no CCK rates available
            if ((pAd->PortCfg.Channel > 14) || (pAd->PortCfg.PhyMode == PHY_11G))
                pAd->PortCfg.TxRate = RATE_6;
            else
            {
                if (dbm >= -82)
                    pAd->PortCfg.TxRate = RATE_11;
                else if (dbm >= -84)
                    pAd->PortCfg.TxRate = RATE_5_5;
                else if (dbm >= -85)
                    pAd->PortCfg.TxRate = RATE_2; 
                else
                    pAd->PortCfg.TxRate = RATE_1; 
            }
        }

        if (pAd->PortCfg.TxRate > pAd->PortCfg.MaxTxRate)
            pAd->PortCfg.TxRate = pAd->PortCfg.MaxTxRate;
            
        return;
    }

    //
    // CASE2. enough TX samples, tune TX rate based on TxPER
    //
    do
    {
        pAd->DrsCounters.CurrTxRateStableTime ++;

        // decide the next upgrade rate and downgrade rate, if any
        if ((pAd->PortCfg.Channel > 14) ||      // must be in 802.11A band
            (pAd->PortCfg.PhyMode == PHY_11G))  // G-only mode, no CCK rates available
        {
            UpRate = Phy11ANextRateUpward[CurrRate];
            DownRate = Phy11ANextRateDownward[CurrRate];
        }
        else
        {
            UpRate = Phy11BGNextRateUpward[CurrRate];
            DownRate = Phy11BGNextRateDownward[CurrRate];
        }

        if (UpRate > pAd->PortCfg.MaxTxRate)
            UpRate = pAd->PortCfg.MaxTxRate;

        TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) *100) / TxTotalCnt;
           
        // downgrade TX quality if PER >= Rate-Down threshold
        if (TxErrorRatio >= RateDownPER[CurrRate])
        {
            pAd->DrsCounters.TxQuality[CurrRate] = DRS_TX_QUALITY_WORST_BOUND;
        }
        // upgrade TX quality if PER <= Rate-Up threshold
        else if (TxErrorRatio <= RateUpPER[CurrRate])
        {
            fUpgradeQuality = TRUE;
            if (pAd->DrsCounters.TxQuality[CurrRate])
                pAd->DrsCounters.TxQuality[CurrRate] --;  // quality very good in CurrRate
                    
            if (pAd->DrsCounters.TxRateUpPenalty)
                pAd->DrsCounters.TxRateUpPenalty --;
            else if (pAd->DrsCounters.TxQuality[UpRate])
                pAd->DrsCounters.TxQuality[UpRate] --;    // may improve next UP rate's quality
        }

        pAd->DrsCounters.PER[CurrRate] = (UCHAR)TxErrorRatio;

#if 1      
        // 2004-3-13 special case: Claim noisy environment
        //   decide if there was a false "rate down" in the past 2 sec due to noisy 
        //   environment. if so, we would rather switch back to the higher TX rate. 
        //   criteria -
        //     1. there's a higher rate available, AND
        //     2. there was a rate-down happened, AND
        //     3. current rate has 75% > PER > 20%, AND
        //     4. comparing to UpRate, current rate didn't improve PER more than 5 %
        if ((UpRate != CurrRate)                              &&
            (pAd->DrsCounters.LastSecTxRateChangeAction == 2) &&
            (pAd->DrsCounters.PER[CurrRate] < 75) && 
            ((pAd->DrsCounters.PER[CurrRate] > 20) || (pAd->DrsCounters.fNoisyEnvironment)) && 
            ((pAd->DrsCounters.PER[CurrRate]+5) > pAd->DrsCounters.PER[UpRate]))
        {
            // we believe this is a noisy environment. better stay at UpRate
            DBGPRINT_RAW(RT_DEBUG_TRACE,"DRS: #### enter Noisy environment ####\n");
            pAd->DrsCounters.fNoisyEnvironment = TRUE;

            // 2004-3-14 when claiming noisy environment, we're not only switch back
            //   to UpRate, but can be more aggressive to use one more rate up
            UpRate++;
            if ((UpRate==RATE_6) || (UpRate==RATE_9)) UpRate=RATE_12;
            if (UpRate > pAd->PortCfg.MaxTxRate)
                UpRate = pAd->PortCfg.MaxTxRate;
            pAd->PortCfg.TxRate = UpRate;
            break;
        }

        // 2004-3-12 special case: Leave noisy environment
        //   The interference has gone suddenly. reset TX rate to
        //   the theoritical value according to RSSI. Criteria -
        //     1. it's currently in noisy environment
        //     2. PER drops to be below 12%
        if ((pAd->DrsCounters.fNoisyEnvironment == TRUE) &&
            (pAd->DrsCounters.PER[CurrRate] <= 12))
        {
            UCHAR JumpUpRate;

            pAd->DrsCounters.fNoisyEnvironment = FALSE;
            for (JumpUpRate = RATE_54; JumpUpRate > RATE_1; JumpUpRate--)
            {
                if (dbm > RssiSafeLevelForTxRate[JumpUpRate])
                    break;
            }

            if (JumpUpRate > pAd->PortCfg.MaxTxRate)
                JumpUpRate = pAd->PortCfg.MaxTxRate;
            
            DBGPRINT_RAW(RT_DEBUG_TRACE,"DRS: #### leave Noisy environment ####, RSSI=%d, JumpUpRate=%d\n",
                dbm, RateIdToMbps[JumpUpRate]);
            
            if (JumpUpRate > CurrRate)
            {
                pAd->PortCfg.TxRate = JumpUpRate;
               	break;
            }
        }
#endif
        // we're going to upgrade CurrRate to UpRate at next few seconds, 
        // but before that, we'd better try a NULL frame @ UpRate and 
        // see if UpRate is stable or not. If this NULL frame fails, it will
        // downgrade TxQuality[CurrRate], so that STA won't switch to
        // to UpRate in the next second
        // 2004-04-07 requested by David Tung - sent test frames only in OFDM rates
        if (fUpgradeQuality      && 
            INFRA_ON(pAd)        && 
            (UpRate != CurrRate) && 
            (UpRate > RATE_11)   &&
            (pAd->DrsCounters.TxQuality[CurrRate] <= 1) &&
            (pAd->DrsCounters.TxQuality[UpRate] <= 1))
        {
            DBGPRINT_RAW(RT_DEBUG_TRACE,"DRS: 2 NULL frames at UpRate = %d Mbps\n",RateIdToMbps[UpRate]);

            if (!(pAd->PortCfg.bAPSDCapable && pAd->PortCfg.APEdcaParm.bAPSDCapable))
			{ 
                RTMPSendNullFrame(pAd, UpRate, FALSE);
                RTMPSendNullFrame(pAd, UpRate, FALSE);
            }
        }

        // perform DRS - consider TxRate Down first, then rate up.
        //     1. rate down, if current TX rate's quality is not good
        //     2. rate up, if UPRate's quality is very good
        if ((pAd->DrsCounters.TxQuality[CurrRate] >= DRS_TX_QUALITY_WORST_BOUND) &&
            (CurrRate != DownRate))
        {
#if 1            
            // guarantee a minimum TX rate for each RSSI segments
            if ((dbm >= -45) && (DownRate < RATE_48))
                pAd->PortCfg.TxRate = RATE_48;
            else if ((dbm >= -50) && (DownRate < RATE_36))
                pAd->PortCfg.TxRate = RATE_36;
            else if ((dbm >= -55) && (DownRate < RATE_24))
                pAd->PortCfg.TxRate = RATE_24;
            else if ((dbm >= -60) && (DownRate < RATE_18))
                pAd->PortCfg.TxRate = RATE_18;
            else if ((dbm >= -65) && (DownRate < RATE_12))
                pAd->PortCfg.TxRate = RATE_12;
            else if ((dbm >= -70) && (DownRate < RATE_9))
			{
				// in 11A or 11G-only mode, no CCK rates available
				if ((pAd->PortCfg.Channel > 14) || (pAd->PortCfg.PhyMode == PHY_11G))
					pAd->PortCfg.TxRate = RATE_9;
				else
					pAd->PortCfg.TxRate = RATE_11;
			}
            else
#endif
            {
                if ((dbm >= -75) && (DownRate < RATE_11))
                    pAd->PortCfg.TxRate = RATE_11;
                else
                {
#ifdef WIFI_TEST
                    if (DownRate <= RATE_2) break; // never goes lower than 5.5 Mbps TX rate
#endif
                    // otherwise, if DownRate still better than the low bound that current RSSI can support,
                    // go straight to DownRate
                    pAd->PortCfg.TxRate = DownRate;
                }
            }
        }
        else if ((pAd->DrsCounters.TxQuality[CurrRate] <= 0) && 
            (pAd->DrsCounters.TxQuality[UpRate] <=0)         &&
            (CurrRate != UpRate))
        {
            pAd->PortCfg.TxRate = UpRate;
        }
        
		//
		// To make sure TxRate didn't over MaxTxRate
		//
		if (pAd->PortCfg.TxRate > pAd->PortCfg.MaxTxRate)
			pAd->PortCfg.TxRate = pAd->PortCfg.MaxTxRate;

    }while (FALSE);


    // if rate-up happen, clear all bad history of all TX rates
    if (pAd->PortCfg.TxRate > CurrRate)
    {
       	DBGPRINT(RT_DEBUG_TRACE,"DRS: ++TX rate from %d to %d Mbps\n", RateIdToMbps[CurrRate],RateIdToMbps[pAd->PortCfg.TxRate]);
        pAd->DrsCounters.CurrTxRateStableTime = 0;
        pAd->DrsCounters.TxRateUpPenalty = 0;
        pAd->DrsCounters.LastSecTxRateChangeAction = 1; // rate UP
        NdisZeroMemory(pAd->DrsCounters.TxQuality, MAX_LEN_OF_SUPPORTED_RATES);
        NdisZeroMemory(pAd->DrsCounters.PER, MAX_LEN_OF_SUPPORTED_RATES);
		//
		// For TxRate fast train up, issued by David 2005/05/12
		// 
		if (!pAd->PortCfg.QuickResponeForRateUpTimerRunning)
		{
			if (pAd->PortCfg.TxRate <= RATE_12)
				RTMPSetTimer(pAd, &pAd->PortCfg.QuickResponeForRateUpTimer, 200);
			else
				RTMPSetTimer(pAd, &pAd->PortCfg.QuickResponeForRateUpTimer, 100);

			pAd->PortCfg.QuickResponeForRateUpTimerRunning = TRUE;
		}
    }
    // if rate-down happen, only clear DownRate's bad history
    else if (pAd->PortCfg.TxRate < CurrRate)
    {
       	DBGPRINT(RT_DEBUG_TRACE,"DRS: --TX rate from %d to %d Mbps\n", RateIdToMbps[CurrRate],RateIdToMbps[pAd->PortCfg.TxRate]);
#if 0
//Remove this code for TxRate fast train up. issued by David 2005/05/12
	    // shorter stable time require more penalty in next rate UP criteria
       	//if (pAd->DrsCounters.CurrTxRateStableTime < 4)      // less then 4 sec
       	//    pAd->DrsCounters.TxRateUpPenalty = DRS_PENALTY; // add 8 sec penalty
       	//else if (pAd->DrsCounters.CurrTxRateStableTime < 8) // less then 8 sec
       	//    pAd->DrsCounters.TxRateUpPenalty = 2;           // add 2 sec penalty
       	//else                                                // >= 8 sec
#endif
       	    pAd->DrsCounters.TxRateUpPenalty = 0;           // no penalty
       	    
        pAd->DrsCounters.CurrTxRateStableTime = 0;
        pAd->DrsCounters.LastSecTxRateChangeAction = 2; // rate DOWN
       	pAd->DrsCounters.TxQuality[pAd->PortCfg.TxRate] = 0;
       	pAd->DrsCounters.PER[pAd->PortCfg.TxRate] = 0;
    }
    else
        pAd->DrsCounters.LastSecTxRateChangeAction = 0; // rate no change
    
}

/*
    ==========================================================================
    Description:
        This routine is executed periodically inside MlmePeriodicExec() after 
        association with an AP.
        It checks if PortCfg.Psm is consistent with user policy (recorded in
        PortCfg.WindowsPowerMode). If not, enforce user policy. However, 
        there're some conditions to consider:
        1. we don't support power-saving in ADHOC mode, so Psm=PWR_ACTIVE all
           the time when Mibss==TRUE
        2. When link up in INFRA mode, Psm should not be switch to PWR_SAVE
           if outgoing traffic available in TxRing or MgmtRing.
    Output:
        1. change pAd->PortCfg.Psm to PWR_SAVE or leave it untouched
        

    ==========================================================================
 */
VOID MlmeCheckPsmChange(
    IN PRTMP_ADAPTER pAd,
    IN ULONG    Now32)
{
	ULONG	PowerMode;
    // condition -
    // 1. Psm maybe ON only happen in INFRASTRUCTURE mode
    // 2. user wants either MAX_PSP or FAST_PSP
    // 3. but current psm is not in PWR_SAVE
    // 4. CNTL state machine is not doing SCANning
    // 5. no TX SUCCESS event for the past 1-sec period
    	PowerMode = pAd->PortCfg.WindowsPowerMode;
    
    if (INFRA_ON(pAd) &&
        (PowerMode != Ndis802_11PowerModeCAM) &&
        (pAd->PortCfg.Psm == PWR_ACTIVE) &&
//      (! RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
        (pAd->Mlme.CntlMachine.CurrState == CNTL_IDLE) &&
        (pAd->RalinkCounters.OneSecTxNoRetryOkCount == 0) &&
        (pAd->RalinkCounters.OneSecTxRetryOkCount == 0))
    {
        MlmeSetPsmBit(pAd, PWR_SAVE);

        if (!(pAd->PortCfg.bAPSDCapable && pAd->PortCfg.APEdcaParm.bAPSDCapable))
		{
			RTMPSendNullFrame(pAd, pAd->PortCfg.TxRate, FALSE);
		}
		else
		{
			RTMPSendNullFrame(pAd, pAd->PortCfg.TxRate, TRUE);
		}
    }

}

VOID MlmeSetPsmBit(
    IN PRTMP_ADAPTER pAd, 
    IN USHORT psm)
{
    TXRX_CSR4_STRUC csr4;
    
    pAd->PortCfg.Psm = psm;    
    RTMP_IO_READ32(pAd, TXRX_CSR4, &csr4.word);
    csr4.field.AckCtsPsmBit = (psm == PWR_SAVE)? 1:0;
    RTMP_IO_WRITE32(pAd, TXRX_CSR4, csr4.word);
    DBGPRINT(RT_DEBUG_TRACE, "MlmeSetPsmBit = %d\n", psm);
}

VOID MlmeSetTxPreamble(
    IN PRTMP_ADAPTER pAd, 
    IN USHORT TxPreamble)
{
    TXRX_CSR4_STRUC csr4;
    
    RTMP_IO_READ32(pAd, TXRX_CSR4, &csr4.word);
    if (TxPreamble == Rt802_11PreambleShort)
    {
        // NOTE: 1Mbps should always use long preamble
        DBGPRINT(RT_DEBUG_TRACE, "MlmeSetTxPreamble (= SHORT PREAMBLE)\n");
        OPSTATUS_SET_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
        csr4.field.AutoResponderPreamble = 1;
    }
    else
    {
        DBGPRINT(RT_DEBUG_TRACE, "MlmeSetTxPreamble (= LONG PREAMBLE)\n");
        OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
        csr4.field.AutoResponderPreamble = 0;
    }
    RTMP_IO_WRITE32(pAd, TXRX_CSR4, csr4.word);
}

// bLinkUp is to identify the inital link speed.
// TRUE indicates the rate update at linkup, we should not try to set the rate at 54Mbps.
VOID MlmeUpdateTxRates(
    IN PRTMP_ADAPTER pAd,
    IN BOOLEAN		 bLinkUp)
{
    int i, num;
    UCHAR Rate, MaxDesire = RATE_1, MaxSupport = RATE_1;
	UCHAR MinSupport = RATE_54;
    ULONG BasicRateBitmap = 0;
    UCHAR CurrBasicRate = RATE_1;
    UCHAR *pSupRate, *pExtRate, SupRateLen, ExtRateLen;

    // find max desired rate
    num = 0;
    for (i=0; i<MAX_LEN_OF_SUPPORTED_RATES; i++)
    {
        switch (pAd->PortCfg.DesireRate[i] & 0x7f)
        {
            case 2:  Rate = RATE_1;   num++;   break;
            case 4:  Rate = RATE_2;   num++;   break;
            case 11: Rate = RATE_5_5; num++;   break;
            case 22: Rate = RATE_11;  num++;   break;
            case 12: Rate = RATE_6;   num++;   break;
            case 18: Rate = RATE_9;   num++;   break;
            case 24: Rate = RATE_12;  num++;   break;
            case 36: Rate = RATE_18;  num++;   break;
            case 48: Rate = RATE_24;  num++;   break;
            case 72: Rate = RATE_36;  num++;   break;
            case 96: Rate = RATE_48;  num++;   break;
            case 108: Rate = RATE_54; num++;   break;
            default: Rate = RATE_1;   break;
        }
        if (MaxDesire < Rate)  MaxDesire = Rate;
    }

    // 2003-12-10 802.11g WIFI spec disallow OFDM rates in 802.11g ADHOC mode
    if ((pAd->PortCfg.BssType == BSS_ADHOC)        &&
        (pAd->PortCfg.PhyMode == PHY_11BG_MIXED)   && 
        (pAd->PortCfg.AdhocMode == 0) &&
        (MaxDesire > RATE_11))
        MaxDesire = RATE_11;
    
    pAd->PortCfg.MaxDesiredRate = MaxDesire;
    
    // Auto rate switching is enabled only if more than one DESIRED RATES are 
    // specified; otherwise disabled
    if (num <= 1)
        OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED);
    else
        OPSTATUS_SET_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED); 
  
    if (ADHOC_ON(pAd) || INFRA_ON(pAd))
    {
        pSupRate = &pAd->ActiveCfg.SupRate[0];
        pExtRate = &pAd->ActiveCfg.ExtRate[0];
        SupRateLen = pAd->ActiveCfg.SupRateLen;
        ExtRateLen = pAd->ActiveCfg.ExtRateLen;
    }
    else
    {
        pSupRate = &pAd->PortCfg.SupRate[0];
        pExtRate = &pAd->PortCfg.ExtRate[0];
        SupRateLen = pAd->PortCfg.SupRateLen;
        ExtRateLen = pAd->PortCfg.ExtRateLen;
    }
  
    // find max supported rate
    for (i=0; i<SupRateLen; i++)
    {   
        switch (pSupRate[i] & 0x7f)
        {
            case 2:   Rate = RATE_1;    if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0001;   break;
            case 4:   Rate = RATE_2;    if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0002;   break;
            case 11:  Rate = RATE_5_5;  if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0004;   break;
            case 22:  Rate = RATE_11;   if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0008;   break;
            case 12:  Rate = RATE_6;    /*if (pSupRate[i] & 0x80)*/  BasicRateBitmap |= 0x0010;  break;
            case 18:  Rate = RATE_9;    if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0020;   break;
            case 24:  Rate = RATE_12;   /*if (pSupRate[i] & 0x80)*/  BasicRateBitmap |= 0x0040;  break;
            case 36:  Rate = RATE_18;   if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0080;   break;
            case 48:  Rate = RATE_24;   /*if (pSupRate[i] & 0x80)*/  BasicRateBitmap |= 0x0100;  break;
            case 72:  Rate = RATE_36;   if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0200;   break;
            case 96:  Rate = RATE_48;   if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0400;   break;
            case 108: Rate = RATE_54;   if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0800;   break;
            default:  Rate = RATE_1;    break;
        }
        if (MaxSupport < Rate)  MaxSupport = Rate;

		if (MinSupport > Rate) MinSupport = Rate;		
    }
    for (i=0; i<ExtRateLen; i++)
    {
        switch (pExtRate[i] & 0x7f)
        {
            case 2:   Rate = RATE_1;    if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0001;   break;
            case 4:   Rate = RATE_2;    if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0002;   break;
            case 11:  Rate = RATE_5_5;  if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0004;   break;
            case 22:  Rate = RATE_11;   if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0008;   break;
            case 12:  Rate = RATE_6;    /*if (pExtRate[i] & 0x80)*/  BasicRateBitmap |= 0x0010;  break;
            case 18:  Rate = RATE_9;    if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0020;   break;
            case 24:  Rate = RATE_12;   /*if (pExtRate[i] & 0x80)*/  BasicRateBitmap |= 0x0040;  break;
            case 36:  Rate = RATE_18;   if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0080;   break;
            case 48:  Rate = RATE_24;   /*if (pExtRate[i] & 0x80)*/  BasicRateBitmap |= 0x0100;  break;
            case 72:  Rate = RATE_36;   if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0200;   break;
            case 96:  Rate = RATE_48;   if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0400;   break;
            case 108: Rate = RATE_54;   if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0800;   break;
            default:  Rate = RATE_1;    break;
        }
        if (MaxSupport < Rate)  MaxSupport = Rate;

		if (MinSupport > Rate) MinSupport = Rate;		
    }
	RTMP_IO_WRITE32(pAd, TXRX_CSR5, BasicRateBitmap);
	pAd->PortCfg.BasicRateBitmap = BasicRateBitmap; 

    // calculate the exptected ACK rate for each TX rate. This info is used to caculate
    // the DURATION field of outgoing uniicast DATA/MGMT frame
    for (i=0; i<MAX_LEN_OF_SUPPORTED_RATES; i++)
    {
        if (BasicRateBitmap & (0x01 << i))
            CurrBasicRate = (UCHAR)i;
        pAd->PortCfg.ExpectedACKRate[i] = CurrBasicRate;
        DBGPRINT(RT_DEBUG_INFO, "Exptected ACK rate[%d] = %d Mbps\n", RateIdToMbps[i], RateIdToMbps[CurrBasicRate]);
    }
        
    // max tx rate = min {max desire rate, max supported rate}
    if (MaxSupport < MaxDesire)
        pAd->PortCfg.MaxTxRate = MaxSupport;
    else
        pAd->PortCfg.MaxTxRate = MaxDesire;

	pAd->PortCfg.MinTxRate = MinSupport;
	
    // 2003-07-31 john - 2500 doesn't have good sensitivity at high OFDM rates. to increase the success
    // ratio of initial DHCP packet exchange, TX rate starts from a lower rate depending
    // on average RSSI
    //   1. RSSI >= -70db, start at 54 Mbps (short distance)
    //   2. -70 > RSSI >= -75, start at 24 Mbps (mid distance)
    //   3. -75 > RSSI, start at 11 Mbps (long distance)
    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED) &&
        OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
    {
        short dbm = pAd->PortCfg.AvgRssi - pAd->BbpRssiToDbmDelta;

     	if ((pAd->RfIcType == RFIC_5325) || (pAd->RfIcType == RFIC_2529))
		{
			if (pAd->PortCfg.AvgRssi2 > pAd->PortCfg.AvgRssi)
			{
				dbm = pAd->PortCfg.AvgRssi2 - pAd->BbpRssiToDbmDelta;
			}
		}
	
		if (bLinkUp == TRUE)
			pAd->PortCfg.TxRate = RATE_24;
		else
        	pAd->PortCfg.TxRate = pAd->PortCfg.MaxTxRate; 

        if (dbm < -75)
            pAd->PortCfg.TxRate = RATE_11;
        else if (dbm < -70)
            pAd->PortCfg.TxRate = RATE_24;

        // should never exceed MaxTxRate (consider 11B-only mode)
        if (pAd->PortCfg.TxRate > pAd->PortCfg.MaxTxRate)
            pAd->PortCfg.TxRate = pAd->PortCfg.MaxTxRate; 
       DBGPRINT(RT_DEBUG_TRACE, " MlmeUpdateTxRates (Rssi=%d, init TX rate = %d Mbps)\n", dbm, RateIdToMbps[pAd->PortCfg.TxRate]);
    }
    else
        pAd->PortCfg.TxRate = pAd->PortCfg.MaxTxRate;


	if (bLinkUp)
	{
		;;//Do nothing
	}
	else
	{
		switch (pAd->PortCfg.PhyMode) 
		{
        case PHY_11BG_MIXED:
        case PHY_11B:
            pAd->PortCfg.MlmeRate = RATE_2;
#ifdef	WIFI_TEST		
            pAd->PortCfg.RtsRate = RATE_11;
#else
            pAd->PortCfg.RtsRate = RATE_2;
#endif
            break;
        case PHY_11A:
            pAd->PortCfg.MlmeRate = RATE_6;
            pAd->PortCfg.RtsRate = RATE_6;
            break;
        case PHY_11ABG_MIXED:
            if (pAd->PortCfg.Channel <= 14)
            {
                pAd->PortCfg.MlmeRate = RATE_2;
                pAd->PortCfg.RtsRate = RATE_2;
            }
            else
            {
                pAd->PortCfg.MlmeRate = RATE_6;
                pAd->PortCfg.RtsRate = RATE_6;
            }
            break;
        default: // error
            pAd->PortCfg.MlmeRate = RATE_2;
            pAd->PortCfg.RtsRate = RATE_2;
            break;
        }
        
		//
		// Keep Basic Mlme Rate.
		//
		pAd->PortCfg.BasicMlmeRate = pAd->PortCfg.MlmeRate;
	}
    
    DBGPRINT(RT_DEBUG_TRACE, " MlmeUpdateTxRates (MaxDesire=%d, MaxSupport=%d, MaxTxRate=%d, MinRate=%d, Rate Switching =%d)\n", 
			 RateIdToMbps[MaxDesire], RateIdToMbps[MaxSupport], RateIdToMbps[pAd->PortCfg.MaxTxRate], RateIdToMbps[pAd->PortCfg.MinTxRate], 
			 OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED));
    DBGPRINT(RT_DEBUG_TRACE, " MlmeUpdateTxRates (TxRate=%d, RtsRate=%d, BasicRateBitmap=0x%04x)\n", 
             RateIdToMbps[pAd->PortCfg.TxRate], RateIdToMbps[pAd->PortCfg.RtsRate], BasicRateBitmap);
}

VOID MlmeRadioOff(
    IN PRTMP_ADAPTER pAd)
{
    DBGPRINT(RT_DEBUG_TRACE, "MlmeRadioOff()\n");
        
    // Set LED
	RTMPSetLED(pAd, LED_RADIO_OFF);
	
	// Set Radio off flag
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF);

	// Link down first if any association exists
	if (INFRA_ON(pAd) || ADHOC_ON(pAd))
		LinkDown(pAd, FALSE);

	// Abort Tx, disable RX, turn off Radio
	RTMP_IO_WRITE32(pAd, TX_CNTL_CSR, 0x001f0000);      // abort all TX rings

	// Disable Rx
	RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);        
		
	RTMP_IO_WRITE32(pAd, MAC_CSR10, 0x00001818);        // Turn off radio

	// Clean up old bss table
	BssTableInit(&pAd->ScanTab);
}


VOID MlmeRadioOn(
    IN PRTMP_ADAPTER pAd)
{	
    DBGPRINT(RT_DEBUG_TRACE,"MlmeRadioOn()\n");
    
	// Turn on radio, Abort TX, Disable RX
	RTMP_IO_WRITE32(pAd, MAC_CSR10, 0x0000071c);        // turn on radio
	RTMP_IO_WRITE32(pAd, TX_CNTL_CSR, 0x001f0000);      // abort all TX rings

	// Disable Rx	
	RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);        

	RTMPRingCleanUp(pAd, QID_AC_BK);
	RTMPRingCleanUp(pAd, QID_AC_BE);
	RTMPRingCleanUp(pAd, QID_AC_VI);
	RTMPRingCleanUp(pAd, QID_AC_VO);
	RTMPRingCleanUp(pAd, QID_HCCA);
	RTMPRingCleanUp(pAd, QID_MGMT);
	RTMPRingCleanUp(pAd, QID_RX);

	NICResetFromError(pAd);

	// Enable Rx
	RTMP_IO_WRITE32(pAd, RX_CNTL_CSR, 0x00000001);  // enable RX of DMA block
    RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x025eb032);	// enable RX of MAC block, Staion not drop control frame


	// Clear Radio off flag
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF);

	// Set LED
	RTMPSetLED(pAd, LED_RADIO_ON);
}

// ===========================================================================================
// bss_table.c
// ===========================================================================================


/*! \brief initialize BSS table
 *  \param p_tab pointer to the table
 *  \return none
 *  \pre
 *  \post
 */
VOID BssTableInit(
    IN BSS_TABLE *Tab) 
{
    int i;

    Tab->BssNr = 0;
    Tab->BssOverlapNr = 0;	
    for (i = 0; i < MAX_LEN_OF_BSS_TABLE; i++) 
    {
        NdisZeroMemory(&Tab->BssEntry[i], sizeof(BSS_ENTRY));
    }
}

/*! \brief search the BSS table by SSID
 *  \param p_tab pointer to the bss table
 *  \param ssid SSID string 
 *  \return index of the table, BSS_NOT_FOUND if not in the table
 *  \pre
 *  \post
 *  \note search by sequential search 
 */
ULONG BssTableSearch(
    IN BSS_TABLE *Tab, 
    IN PUCHAR    pBssid,
    IN UCHAR     Channel) 
{
    UCHAR i;
    
    for (i = 0; i < Tab->BssNr; i++) 
    {
    	//
    	// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
    	// We should distinguish this case.
    	//    	
        if ((((Tab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			 ((Tab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(Tab->BssEntry[i].Bssid, pBssid)) 
        { 
            return i;
        }
    }
    return (ULONG)BSS_NOT_FOUND;
}

ULONG BssSsidTableSearch(
	IN BSS_TABLE *Tab, 
	IN PUCHAR    pBssid,
	IN PUCHAR    pSsid,
	IN UCHAR     SsidLen,
	IN UCHAR     Channel) 
{
	UCHAR i;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		//
		// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
		// We should distinguish this case.
		//    	
		if ((((Tab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			 ((Tab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(Tab->BssEntry[i].Bssid, pBssid) &&
			SSID_EQUAL(pSsid, SsidLen, Tab->BssEntry[i].Ssid, Tab->BssEntry[i].SsidLen)) 
		{ 
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}

ULONG BssTableSearchWithSSID(
	IN BSS_TABLE *Tab, 
	IN PUCHAR    Bssid,
	IN PUCHAR    pSsid,
	IN UCHAR     SsidLen,
	IN UCHAR     Channel)
{
	UCHAR i;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		if ((((Tab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			((Tab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(&(Tab->BssEntry[i].Bssid), Bssid) &&
			(SSID_EQUAL(pSsid, SsidLen, Tab->BssEntry[i].Ssid, Tab->BssEntry[i].SsidLen) ||
			(NdisEqualMemory(pSsid, ZeroSsid, SsidLen)) || 
			(NdisEqualMemory(Tab->BssEntry[i].Ssid, ZeroSsid, Tab->BssEntry[i].SsidLen))))
		{ 
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}

VOID BssTableDeleteEntry(
	IN OUT	BSS_TABLE *Tab, 
	IN		PUCHAR    pBssid,
	IN      UCHAR     Channel)
{
	UCHAR i, j;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		//printf("comparing %s and %s\n", p_tab->bss[i].ssid, ssid);
		if ((Tab->BssEntry[i].Channel == Channel) && 
			(MAC_ADDR_EQUAL(Tab->BssEntry[i].Bssid, pBssid)))
		{
			for (j = i; j < Tab->BssNr - 1; j++)
			{
				NdisMoveMemory(&(Tab->BssEntry[j]), &(Tab->BssEntry[j + 1]), sizeof(BSS_ENTRY));
			}
			Tab->BssNr -= 1;
			return;
		}
	}
}

/*! \brief
 *  \param 
 *  \return
 *  \pre
 *  \post
 */
VOID BssEntrySet(
    IN  PRTMP_ADAPTER   pAd, 
    OUT BSS_ENTRY *pBss, 
    IN PUCHAR pBssid, 
    IN CHAR Ssid[], 
    IN UCHAR SsidLen, 
    IN UCHAR BssType, 
    IN USHORT BeaconPeriod, 
    IN PCF_PARM pCfParm, 
    IN USHORT AtimWin, 
    IN USHORT CapabilityInfo, 
    IN UCHAR SupRate[], 
    IN UCHAR SupRateLen,
    IN UCHAR ExtRate[], 
    IN UCHAR ExtRateLen,
    IN UCHAR Channel,
    IN UCHAR Rssi,
    IN LARGE_INTEGER TimeStamp,
    IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
//	IN UCHAR LengthVIE,
	IN USHORT LengthVIE,  // edit by johnli, variable ie length could be > 256
    IN PNDIS_802_11_VARIABLE_IEs pVIE)  
{
    pBss->LastBeaconRxTime = jiffies; // Update it at here to fix always "0"
    
    COPY_MAC_ADDR(pBss->Bssid, pBssid);
    // Default Hidden SSID to be TRUE, it will be turned to FALSE after coping SSID
    pBss->Hidden = 1;   
    if (SsidLen > 0)
    {
        // For hidden SSID AP, it might send beacon with SSID len equal to 0
        // Or send beacon /probe response with SSID len matching real SSID length,
        // but SSID is all zero. such as "00-00-00-00" with length 4.
        // We have to prevent this case overwrite correct table
        if (NdisEqualMemory(Ssid, ZeroSsid, SsidLen) == 0)
        {
            NdisMoveMemory(pBss->Ssid, Ssid, SsidLen);
            pBss->SsidLen = SsidLen;
            pBss->Hidden = 0;
        }
    }
    pBss->BssType = BssType;
    pBss->BeaconPeriod = BeaconPeriod;
    if (BssType == BSS_INFRA) 
    {
        if (pCfParm->bValid) 
        {
            pBss->CfpCount = pCfParm->CfpCount;
            pBss->CfpPeriod = pCfParm->CfpPeriod;
            pBss->CfpMaxDuration = pCfParm->CfpMaxDuration;
            pBss->CfpDurRemaining = pCfParm->CfpDurRemaining;
        }
    } 
    else 
    {
        pBss->AtimWin = AtimWin;
    }

    pBss->CapabilityInfo = CapabilityInfo;
    // The privacy bit indicate security is ON, it maight be WEP, TKIP or AES
    // Combine with AuthMode, they will decide the connection methods.
    pBss->Privacy = CAP_IS_PRIVACY_ON(pBss->CapabilityInfo);
    NdisMoveMemory(pBss->SupRate, SupRate, SupRateLen);
    pBss->SupRateLen = SupRateLen;
    NdisMoveMemory(pBss->ExtRate, ExtRate, ExtRateLen);
    pBss->ExtRateLen = ExtRateLen;
    pBss->Channel = Channel;
    pBss->Rssi = Rssi;
    // Update CkipFlag. if not exists, the value is 0x0
	pBss->CkipFlag = CkipFlag;

    // New for microsoft Fixed IEs
    NdisMoveMemory(pBss->FixIEs.Timestamp, &TimeStamp, 8);
    pBss->FixIEs.BeaconInterval = BeaconPeriod;
    pBss->FixIEs.Capabilities = CapabilityInfo;

    if (LengthVIE != 0)
	{
		pBss->VarIELen = LengthVIE;
		NdisMoveMemory(pBss->VarIEs, pVIE, pBss->VarIELen);
	}
	else
	{
		pBss->VarIELen = 0;
	}
    
	BssCipherParse(pBss);
	
	// new for QOS
	if (pEdcaParm)
	    NdisMoveMemory(&pBss->EdcaParm, pEdcaParm, sizeof(EDCA_PARM));
	else
	    pBss->EdcaParm.bValid = FALSE;
	if (pQosCapability)
	    NdisMoveMemory(&pBss->QosCapability, pQosCapability, sizeof(QOS_CAPABILITY_PARM));
	else
	    pBss->QosCapability.bValid = FALSE;
	if (pQbssLoad)
	    NdisMoveMemory(&pBss->QbssLoad, pQbssLoad, sizeof(QBSS_LOAD_PARM));
	else
	    pBss->QbssLoad.bValid = FALSE;
}

/*! 
 *  \brief insert an entry into the bss table
 *  \param p_tab The BSS table
 *  \param Bssid BSSID
 *  \param ssid SSID
 *  \param ssid_len Length of SSID
 *  \param bss_type
 *  \param beacon_period
 *  \param timestamp
 *  \param p_cf
 *  \param atim_win
 *  \param cap
 *  \param rates
 *  \param rates_len
 *  \param channel_idx
 *  \return none
 *  \pre
 *  \post
 *  \note If SSID is identical, the old entry will be replaced by the new one
 */
ULONG BssTableSetEntry(
    IN  PRTMP_ADAPTER   pAd, 
    OUT BSS_TABLE *Tab, 
    IN PUCHAR pBssid, 
    IN CHAR Ssid[], 
    IN UCHAR SsidLen, 
    IN UCHAR BssType, 
    IN USHORT BeaconPeriod, 
    IN CF_PARM *CfParm, 
    IN USHORT AtimWin, 
    IN USHORT CapabilityInfo, 
    IN UCHAR SupRate[],
    IN UCHAR SupRateLen,
    IN UCHAR ExtRate[],
    IN UCHAR ExtRateLen,
    IN UCHAR ChannelNo,
    IN UCHAR Rssi,
    IN LARGE_INTEGER TimeStamp,
    IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
//	IN UCHAR LengthVIE,
	IN USHORT LengthVIE,  // edit by johnli, variable ie length could be > 256
    IN PNDIS_802_11_VARIABLE_IEs pVIE) 
{
    ULONG   Idx;

	Idx = BssTableSearchWithSSID(Tab, pBssid,  Ssid, SsidLen, ChannelNo);
    if (Idx == BSS_NOT_FOUND) 
    {
        if (Tab->BssNr >= MAX_LEN_OF_BSS_TABLE)
	{
		//
		// It may happen when BSS Table was full.
		// The desired AP will not be added into BSS Table
		// In this case, if we found the desired AP then overwrite BSS Table.
		//
		if(!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
		{
			if (MAC_ADDR_EQUAL(pAd->MlmeAux.Bssid, pBssid) ||
				SSID_EQUAL(pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen, Ssid, SsidLen))
			{
				Idx = Tab->BssOverlapNr;
				DBGPRINT(RT_DEBUG_TRACE, "===OverWrite ScanTab Idx:%d===\n",Idx);
				BssEntrySet(pAd, &Tab->BssEntry[Idx], pBssid, Ssid, SsidLen, BssType, BeaconPeriod,
							CfParm, AtimWin, CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,
							ChannelNo, Rssi, TimeStamp, CkipFlag, pEdcaParm, pQosCapability, pQbssLoad, LengthVIE, pVIE);
				Tab->BssOverlapNr = (Tab->BssOverlapNr++) % MAX_LEN_OF_BSS_TABLE;
			}
			return Idx;
		}
		else
            return BSS_NOT_FOUND;
	}
        Idx = Tab->BssNr;
        BssEntrySet(pAd, &Tab->BssEntry[Idx], pBssid, Ssid, SsidLen, BssType, BeaconPeriod,
                    CfParm, AtimWin, CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,
                    ChannelNo, Rssi, TimeStamp, CkipFlag, pEdcaParm, pQosCapability, pQbssLoad, LengthVIE, pVIE);
        Tab->BssNr++;
    } 
    else
    {
        BssEntrySet(pAd, &Tab->BssEntry[Idx], pBssid, Ssid, SsidLen, BssType, BeaconPeriod,
                    CfParm, AtimWin, CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,
                    ChannelNo, Rssi, TimeStamp, CkipFlag, pEdcaParm, pQosCapability, pQbssLoad, LengthVIE, pVIE);
    }
    
    return Idx;
}

BOOLEAN RTMPCheckAKM(
    IN NDIS_802_11_AUTHENTICATION_MODE auth, 
    IN BSS_ENTRY *pBss)
{
    switch (auth) {
    case Ndis802_11AuthModeWPA:
    	if(pBss->AuthBitMode & WPA1AKMBIT)
	    return TRUE;
	else 
	    return FALSE;
    case Ndis802_11AuthModeWPAPSK:
    	if(pBss->AuthBitMode & WPA1PSKAKMBIT)
	    return TRUE;
	else
	    return FALSE;
    case Ndis802_11AuthModeWPA2:
       	if(pBss->AuthBitMode & WPA2AKMBIT)
	    return TRUE;
	else
	    return FALSE;
    case Ndis802_11AuthModeWPA2PSK:
        if(pBss->AuthBitMode & WPA2PSKAKMBIT)
	    return TRUE;
	else
	    return FALSE;
    default:
        return FALSE;
    }			           	             	    	        	        
}

VOID BssTableSsidSort(
    IN	PRTMP_ADAPTER	pAd, 
    OUT BSS_TABLE *OutTab, 
    IN  CHAR Ssid[], 
    IN  UCHAR SsidLen) 
{
    INT i;
    BssTableInit(OutTab);

    for (i = 0; i < pAd->ScanTab.BssNr; i++) 
    {
        BSS_ENTRY *pInBss = &pAd->ScanTab.BssEntry[i];
        if ((pInBss->BssType == pAd->PortCfg.BssType) &&
//            !strncmp(Ssid, pInBss->Ssid, pInBss->SsidLen))
            SSID_EQUAL(Ssid, SsidLen, pInBss->Ssid, pInBss->SsidLen)) // edit by johnli, fix the bug to get a empty ssid entry (disable ssid broadcast)
        {
            BSS_ENTRY *pOutBss = &OutTab->BssEntry[OutTab->BssNr];


			// New for WPA2
			// Check the Authmode first
			if (pAd->PortCfg.AuthMode >= Ndis802_11AuthModeWPA)
			{
  
                if (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPANone)
                {
                    if (pAd->PortCfg.AuthMode != pInBss->AuthMode)
                        continue;// None matched 
                }
                else
                {
                    // Check AuthMode and AuthBitMode for matching, in case AP support dual-mode
					if(!RTMPCheckAKM(pAd->PortCfg.AuthMode,pInBss))
              			continue;// None matched
                }
				
				// Check cipher suite, AP must have more secured cipher than station setting
				if ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA) || (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPAPSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipherAux))
						continue;						
				}
				else if ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA2) || (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA2PSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA2.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA2.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA2.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipherAux))
						continue;						
				}
			}			
					// unused: Suzhou update. Skip the auth method check, it is only for AP(Apple Airport Extreme 11n)
					// unused: The apple AP will recommend client use WPA/WPA2 auth to it when its setting wep-128.
			//patch for wifi
			// Bss Type matched, SSID matched. 
			// We will check wepstatus for qualification Bss
			else if (pAd->PortCfg.WepStatus != pInBss->WepStatus)
			{
                DBGPRINT(RT_DEBUG_TRACE,"PortCfg.WepStatus=%d, while pInBss->WepStatus=%d\n", pAd->PortCfg.WepStatus, pInBss->WepStatus);
				continue;
			}
			// Since the AP is using hidden SSID, and we are trying to connect to ANY
			// It definitely will fail. So, skip it.
			// CCX also require not even try to connect it!!
			if (SsidLen == 0)
				continue;
			
            // copy matching BSS from InTab to OutTab
            NdisMoveMemory(pOutBss, pInBss, sizeof(BSS_ENTRY));
            
            OutTab->BssNr++;
        }
        else if ((pInBss->BssType == pAd->PortCfg.BssType) && (SsidLen == 0))
        {
            BSS_ENTRY *pOutBss = &OutTab->BssEntry[OutTab->BssNr];

			// New for WPA2
			// Check the Authmode first
			if (pAd->PortCfg.AuthMode >= Ndis802_11AuthModeWPA)
			{
				// Check AuthMode and AuthBitMode for matching, in case AP support dual-mode
				if(!RTMPCheckAKM(pAd->PortCfg.AuthMode,pInBss))
					continue;   // None matched
				
				// Check cipher suite, AP must have more secured cipher than station setting
				if ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA) || (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPAPSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA.PairCipherAux))
						continue;						
				}
				else if ((pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA2) || (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA2PSK))
				{
					// If it's not mixed mode, we should only let BSS pass with the same encryption
					if (pInBss->WPA2.bMixMode == FALSE)
						if (pAd->PortCfg.WepStatus != pInBss->WPA2.GroupCipher)
							continue;
						
					// check group cipher
					if (pAd->PortCfg.WepStatus < pInBss->WPA2.GroupCipher)
						continue;

					// check pairwise cipher, skip if none matched
					// If profile set to AES, let it pass without question.
					// If profile set to TKIP, we must find one mateched
					if ((pAd->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipher) && 
						(pAd->PortCfg.WepStatus != pInBss->WPA2.PairCipherAux))
						continue;						
				}
			}
			// Bss Type matched, SSID matched. 
			// We will check wepstatus for qualification Bss
			else if (pAd->PortCfg.WepStatus != pInBss->WepStatus)
					continue;
			
            // copy matching BSS from InTab to OutTab
            NdisMoveMemory(pOutBss, pInBss, sizeof(BSS_ENTRY));
            
            OutTab->BssNr++;
        }
        
		if (OutTab->BssNr >= MAX_LEN_OF_BSS_TABLE)
			break;
		
    }
    
    BssTableSortByRssi(OutTab);


    if (OutTab->BssNr > 0)
	{
	    if (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPA2PSK)
			RTMPMakeRSNIE(pAd, OutTab->BssEntry[0].WPA2.GroupCipher);
		else if (pAd->PortCfg.AuthMode == Ndis802_11AuthModeWPAPSK)
			RTMPMakeRSNIE(pAd, OutTab->BssEntry[0].WPA.GroupCipher);
	}

}

VOID BssTableSortByRssi(
    IN OUT BSS_TABLE *OutTab) 
{
    INT       i, j;
    BSS_ENTRY TmpBss;

    for (i = 0; i < OutTab->BssNr - 1; i++) 
    {
        for (j = i+1; j < OutTab->BssNr; j++) 
        {
            if (OutTab->BssEntry[j].Rssi > OutTab->BssEntry[i].Rssi) 
            {
                NdisMoveMemory(&TmpBss, &OutTab->BssEntry[j], sizeof(BSS_ENTRY));
                NdisMoveMemory(&OutTab->BssEntry[j], &OutTab->BssEntry[i], sizeof(BSS_ENTRY));
                NdisMoveMemory(&OutTab->BssEntry[i], &TmpBss, sizeof(BSS_ENTRY));
            }
        }
    }
}

extern	UCHAR	RSN_OUI[];		// in sanity.c
VOID BssCipherParse(
	IN OUT	PBSS_ENTRY	pBss)
{
	PEID_STRUCT          pEid;
	PUCHAR				pTmp;
	PRSN_IE_HEADER_STRUCT			pRsnHeader;
	PCIPHER_SUITE_STRUCT			pCipher;
	PAKM_SUITE_STRUCT				pAKM;
	USHORT							Count;
	INT								Length;
	NDIS_802_11_ENCRYPTION_STATUS	TmpCipher;

	//
	// WepStatus will be reset later, if AP announce TKIP or AES on the beacon frame.
	//
	if (pBss->Privacy)
	{
		pBss->WepStatus     = Ndis802_11WEPEnabled;
	}
	else
	{
		pBss->WepStatus     = Ndis802_11WEPDisabled;
	}
	// Set default to disable & open authentication before parsing variable IE
	pBss->AuthMode      = Ndis802_11AuthModeOpen;
	pBss->AuthModeAux   = Ndis802_11AuthModeOpen;
	pBss->AuthBitMode = 0;
	pBss->PairCipherBitMode = 0;

	// Init WPA setting
	pBss->WPA.PairCipher    = Ndis802_11WEPDisabled;
	pBss->WPA.PairCipherAux = Ndis802_11WEPDisabled;
	pBss->WPA.GroupCipher   = Ndis802_11WEPDisabled;
	pBss->WPA.RsnCapability = 0;
	pBss->WPA.bMixMode      = FALSE;

	// Init WPA2 setting
	pBss->WPA2.PairCipher    = Ndis802_11WEPDisabled;
	pBss->WPA2.PairCipherAux = Ndis802_11WEPDisabled;
	pBss->WPA2.GroupCipher   = Ndis802_11WEPDisabled;
	pBss->WPA2.RsnCapability = 0;
	pBss->WPA2.bMixMode      = FALSE;
	Length = (INT) pBss->VarIELen;

	while (Length > 0)
	{
		// Parse cipher suite base on WPA1 & WPA2, they should be parsed differently
		pTmp = ((PUCHAR) pBss->VarIEs) + pBss->VarIELen - Length;
		pEid = (PEID_STRUCT) pTmp;
		switch (pEid->Eid)
		{
			case IE_WPA:
				if (NdisEqualMemory(pEid->Octet, WPA_OUI, 4) != 1)
				{
					// if unsupported vendor specific IE
					break;
				}				
	            // Skip OUI, version, and multicast suite
	            // This part should be improved in the future when AP supported multiple cipher suite.
	            // For now, it's OK since almost all APs have fixed cipher suite supported.
				// pTmp = (PUCHAR) pEid->Octet;
				pTmp   += 11;

	            // Cipher Suite Selectors from Spec P802.11i/D3.2 P26.
	            //  Value      Meaning
	            //  0           None 
	            //  1           WEP-40
	            //  2           Tkip
	            //  3           WRAP
	            //  4           AES
	            //  5           WEP-104
				// Parse group cipher
				switch (*pTmp)
				{
					case 1:
					case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
						pBss->WPA.GroupCipher = Ndis802_11Encryption1Enabled;
						break;
					case 2:
						pBss->WPA.GroupCipher = Ndis802_11Encryption2Enabled;
						break;
					case 4:
						pBss->WPA.GroupCipher = Ndis802_11Encryption3Enabled;
						break;
					default:
						break;
				}
				// number of unicast suite
				pTmp   += 1;

				// skip all unicast cipher suites
				//Count = *(PUSHORT) pTmp;
			    NdisMoveMemory(&Count, pTmp, sizeof(USHORT));	
#ifdef BIG_ENDIAN
				Count = SWAP16(Count);		
#endif
				pTmp   += sizeof(USHORT);

				// Parsing all unicast cipher suite
				while (Count > 0)
				{
					// Skip OUI
					pTmp += 3;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (*pTmp)
					{
						case 1:
						case 5: // Although WEP is not allowed in WPA related auth mode, we parse it anyway
							TmpCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							TmpCipher = Ndis802_11Encryption2Enabled;
							pBss->PairCipherBitMode |= TKIPBIT;
							break;
						case 4:
							TmpCipher = Ndis802_11Encryption3Enabled;
							pBss->PairCipherBitMode |= CCMPBIT;
							break;
						default:
							break;
					}
					if (TmpCipher > pBss->WPA.PairCipher)
					{
						// Move the lower cipher suite to PairCipherAux
						pBss->WPA.PairCipherAux = pBss->WPA.PairCipher;
						pBss->WPA.PairCipher    = TmpCipher;
					}
					else
					{
						pBss->WPA.PairCipherAux = TmpCipher;
					}
					pTmp++;
					Count--;
				}
				
				// 4. get AKM suite counts
				//Count   = *(PUSHORT) pTmp;
				NdisMoveMemory(&Count, pTmp, sizeof(USHORT));	
#ifdef BIG_ENDIAN
				Count = SWAP16(Count);		
#endif
				pTmp   += sizeof(USHORT);


			    while(Count >0){
			    
				pTmp   += 3;
				
				switch (*pTmp)
				{
					case 1:
						// Set AP support WPA1 mode
						pBss->AuthMode = Ndis802_11AuthModeWPA;
						pBss->AuthBitMode|=WPA1AKMBIT;
						break;
					case 2:
						// Set AP support WPA1PSK
						pBss->AuthMode = Ndis802_11AuthModeWPAPSK;
						pBss->AuthBitMode|=WPA1PSKAKMBIT;
				    	
						break;
					default:
						break;
				}
				    pTmp++;
					Count--;
				}



				// Fixed for WPA-None
				if (pBss->BssType == BSS_ADHOC)
				{
					pBss->AuthMode    = Ndis802_11AuthModeWPANone;
					pBss->AuthModeAux = Ndis802_11AuthModeWPANone;
					pBss->WepStatus   = pBss->WPA.GroupCipher;
					// Patched bugs for old driver
					if (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled)
						pBss->WPA.PairCipherAux = pBss->WPA.GroupCipher;
				}
				else
					pBss->WepStatus   = pBss->WPA.PairCipher;					
				
				// Check the Pair & Group, if different, turn on mixed mode flag
				if (pBss->WPA.GroupCipher != pBss->WPA.PairCipher)
					pBss->WPA.bMixMode = TRUE;
				
				break;

			case IE_RSN:
				pRsnHeader = (PRSN_IE_HEADER_STRUCT) pTmp;
				
				// 0. Version must be 1
#ifdef BIG_ENDIAN
				pRsnHeader->Version = SWAP16(pRsnHeader->Version);
#endif								
				if (pRsnHeader->Version != 1)
					break;
				pTmp   += sizeof(RSN_IE_HEADER_STRUCT);

				// 1. Check group cipher
				pCipher = (PCIPHER_SUITE_STRUCT) pTmp;
				if (!RTMPEqualMemory(pTmp, RSN_OUI, 3))
					break;

				// Parse group cipher
				switch (pCipher->Type)
				{
					case 1:
					case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
						pBss->WPA2.GroupCipher = Ndis802_11Encryption1Enabled;
						break;
					case 2:
						pBss->WPA2.GroupCipher = Ndis802_11Encryption2Enabled;
						break;
					case 4:
						pBss->WPA2.GroupCipher = Ndis802_11Encryption3Enabled;
						break;
					default:
						break;
				}
				// set to correct offset for next parsing
				pTmp   += sizeof(CIPHER_SUITE_STRUCT);

				// 2. Get pairwise cipher counts
				//Count = *(PUSHORT) pTmp;
				NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
#ifdef BIG_ENDIAN
				Count = SWAP16(Count);		
#endif
				pTmp   += sizeof(USHORT);
				
				// 3. Get pairwise cipher
				// Parsing all unicast cipher suite
				while (Count > 0)
				{
					// Skip OUI
					pCipher = (PCIPHER_SUITE_STRUCT) pTmp;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (pCipher->Type)
					{
						case 1:
						case 5: // Although WEP is not allowed in WPA related auth mode, we parse it anyway
							TmpCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							TmpCipher = Ndis802_11Encryption2Enabled;
							pBss->PairCipherBitMode |= TKIPBIT;
							break;
						case 4:
							TmpCipher = Ndis802_11Encryption3Enabled;
							pBss->PairCipherBitMode |= CCMPBIT;
							break;
						default:
							break;
					}
					if (TmpCipher > pBss->WPA2.PairCipher)
					{
						// Move the lower cipher suite to PairCipherAux
						pBss->WPA2.PairCipherAux = pBss->WPA2.PairCipher;
						pBss->WPA2.PairCipher    = TmpCipher;
					}
					else
					{
						pBss->WPA2.PairCipherAux = TmpCipher;
					}
					pTmp += sizeof(CIPHER_SUITE_STRUCT);
					Count--;
				}
				
				// 4. get AKM suite counts
				//Count   = *(PUSHORT) pTmp;
				NdisMoveMemory(&Count, pTmp, sizeof(USHORT));
#ifdef BIG_ENDIAN
				Count = SWAP16(Count);		
#endif
				pTmp   += sizeof(USHORT);

				// 5. Get AKM ciphers
				pAKM = (PAKM_SUITE_STRUCT) pTmp;
				if (!RTMPEqualMemory(pTmp, RSN_OUI, 3))
					break;

			   while(Count >0){
			       
				    pTmp += 3;

				    switch (*pTmp)
				{
					case 1:
						// Set AP support WPA2 mode
						pBss->AuthMode = Ndis802_11AuthModeWPA2;
						pBss->AuthBitMode |= WPA2AKMBIT;
						break;
					case 2:
						// Set AP support WPA2PSK mode
						pBss->AuthMode = Ndis802_11AuthModeWPA2PSK;
						pBss->AuthBitMode |= WPA2PSKAKMBIT;
						break;
					default:
						break;
				    	    
				}
				    pTmp ++;
				    Count --;
				}
				//pTmp   += (Count * sizeof(AKM_SUITE_STRUCT));

				// Fixed for WPA-None
				if (pBss->BssType == BSS_ADHOC)
				{
					pBss->AuthMode = Ndis802_11AuthModeWPANone;
					pBss->AuthModeAux = Ndis802_11AuthModeWPANone;
					pBss->WPA.PairCipherAux = pBss->WPA2.PairCipherAux;
					pBss->WPA.GroupCipher   = pBss->WPA2.GroupCipher;
					pBss->WepStatus         = pBss->WPA.GroupCipher;
					// Patched bugs for old driver
					if (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled)
						pBss->WPA.PairCipherAux = pBss->WPA.GroupCipher;
				}								
				pBss->WepStatus   = pBss->WPA2.PairCipher;
				
				// 6. Get RSN capability
				//pBss->WPA2.RsnCapability = *(PUSHORT) pTmp;			
				NdisMoveMemory(&pBss->WPA2.RsnCapability, pTmp, sizeof(USHORT));
#ifdef BIG_ENDIAN
				pBss->WPA2.RsnCapability = SWAP16(pBss->WPA2.RsnCapability);		
#endif
				pTmp += sizeof(USHORT);
				
				// Check the Pair & Group, if different, turn on mixed mode flag
				if (pBss->WPA2.GroupCipher != pBss->WPA2.PairCipher)
					pBss->WPA2.bMixMode = TRUE;
				
				break;

			default:
				break;
		}
		Length -= (pEid->Len + 2);
	}
}


// ===========================================================================================
// mac_table.c
// ===========================================================================================

/*! \brief generates a random mac address value for IBSS BSSID
 *  \param Addr the bssid location
 *  \return none
 *  \pre
 *  \post
 */
VOID MacAddrRandomBssid(
    IN PRTMP_ADAPTER pAd, 
    OUT PUCHAR pAddr) 
{
    INT i;

    for (i = 0; i < MAC_ADDR_LEN; i++) 
    {
        pAddr[i] = RandomByte(pAd);
    }
    
    pAddr[0] = (pAddr[0] & 0xfe) | 0x02;  // the first 2 bits must be 01xxxxxxxx
}

/*! \brief init the management mac frame header
 *  \param p_hdr mac header
 *  \param subtype subtype of the frame
 *  \param p_ds destination address, don't care if it is a broadcast address
 *  \return none
 *  \pre the station has the following information in the pAd->UserCfg
 *   - bssid
 *   - station address
 *  \post
 *  \note this function initializes the following field
 */
VOID MgtMacHeaderInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN OUT PHEADER_802_11 pHdr80211, 
    IN UCHAR SubType, 
    IN UCHAR ToDs, 
    IN PUCHAR pDA, 
    IN PUCHAR pBssid) 
{
    NdisZeroMemory(pHdr80211, sizeof(HEADER_802_11));
    pHdr80211->FC.Type = BTYPE_MGMT;
    pHdr80211->FC.SubType = SubType;
    pHdr80211->FC.ToDs = ToDs;
    COPY_MAC_ADDR(pHdr80211->Addr1, pDA);
    COPY_MAC_ADDR(pHdr80211->Addr2, pAd->CurrentAddress);
    COPY_MAC_ADDR(pHdr80211->Addr3, pBssid);
}

// ===========================================================================================
// mem_mgmt.c
// ===========================================================================================

/*!***************************************************************************
 * This routine build an outgoing frame, and fill all information specified 
 * in argument list to the frame body. The actual frame size is the summation 
 * of all arguments.
 * input params:
 *      Buffer - pointer to a pre-allocated memory segment
 *      args - a list of <int arg_size, arg> pairs.
 *      NOTE NOTE NOTE!!!! the last argument must be NULL, otherwise this
 *                         function will FAIL!!!
 * return:
 *      Size of the buffer
 * usage:  
 *      MakeOutgoingFrame(Buffer, output_length, 2, &fc, 2, &dur, 6, p_addr1, 6,p_addr2, END_OF_ARGS);
 ****************************************************************************/
ULONG MakeOutgoingFrame(
    OUT CHAR *Buffer, 
    OUT ULONG *FrameLen, ...) 
{
    CHAR   *p;
    int     leng;
    ULONG   TotLeng;
    va_list Args;
         
    // calculates the total length
    TotLeng = 0;
    va_start(Args, FrameLen);
    do 
    {
        leng = va_arg(Args, int);
        if (leng == END_OF_ARGS) 
        {
            break;
        }
        p = va_arg(Args, PVOID);
        NdisMoveMemory(&Buffer[TotLeng], p, leng);
        TotLeng = TotLeng + leng;
    } while(TRUE);

    va_end(Args); /* clean up */
    *FrameLen = TotLeng;
    return TotLeng;
}

// ===========================================================================================
// mlme_queue.c
// ===========================================================================================

/*! \brief  Initialize The MLME Queue, used by MLME Functions
 *  \param  *Queue     The MLME Queue
 *  \return Always     Return NDIS_STATE_SUCCESS in this implementation
 *  \pre
 *  \post
 *  \note   Because this is done only once (at the init stage), no need to be locked
 */
NDIS_STATUS MlmeQueueInit(
    IN MLME_QUEUE *Queue) 
{
    INT i;

    NdisAllocateSpinLock(&Queue->Lock);

    Queue->Num  = 0;
    Queue->Head = 0;
    Queue->Tail = 0;

    for (i = 0; i < MAX_LEN_OF_MLME_QUEUE; i++) 
    {
        Queue->Entry[i].Occupied = FALSE;
        Queue->Entry[i].MsgLen = 0;
        NdisZeroMemory(Queue->Entry[i].Msg, MAX_LEN_OF_MLME_BUFFER);
    }

    return NDIS_STATUS_SUCCESS;
}

/*! \brief   Enqueue a message for other threads, if they want to send messages to MLME thread
 *  \param  *Queue    The MLME Queue
 *  \param   Machine  The State Machine Id
 *  \param   MsgType  The Message Type
 *  \param   MsgLen   The Message length
 *  \param  *Msg      The message pointer
 *  \return  TRUE if enqueue is successful, FALSE if the queue is full
 *  \pre
 *  \post
 *  \note    The message has to be initialized
 */
BOOLEAN MlmeEnqueue(
	IN	PRTMP_ADAPTER	pAd,
    IN ULONG Machine, 
    IN ULONG MsgType, 
    IN ULONG MsgLen, 
    IN VOID *Msg) 
{
    INT Tail;
	MLME_QUEUE	*Queue = (MLME_QUEUE *)&pAd->Mlme.Queue;
    unsigned long	IrqFlags;   
	

    // Do nothing if the driver is starting halt state.
    // This might happen when timer already been fired before cancel timer with mlmehalt
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
        return FALSE;

	// First check the size, it MUST not exceed the mlme queue size
	if (MsgLen > MAX_LEN_OF_MLME_BUFFER)
	{
        DBGPRINT_ERR("MlmeEnqueue: msg too large, size = %d \n", MsgLen);
		return FALSE;
	}
	
    if (MlmeQueueFull(Queue)) 
    {
        DBGPRINT_ERR("MlmeEnqueue: full, msg dropped and may corrupt MLME\n");
        return FALSE;
    }

	RTMP_SEM_LOCK(&(Queue->Lock), IrqFlags);

    Tail = Queue->Tail;
    Queue->Tail++;
    Queue->Num++;
    if (Queue->Tail == MAX_LEN_OF_MLME_QUEUE) 
    {
        Queue->Tail = 0;
    }

    DBGPRINT(RT_DEBUG_INFO, "MlmeEnqueue, num=%d\n",Queue->Num);
 
    Queue->Entry[Tail].Occupied = TRUE;
    Queue->Entry[Tail].Machine = Machine;
    Queue->Entry[Tail].MsgType = MsgType;
    Queue->Entry[Tail].MsgLen  = MsgLen;
    NdisMoveMemory(Queue->Entry[Tail].Msg, Msg, MsgLen);

	RTMP_SEM_UNLOCK(&(Queue->Lock), IrqFlags);

    return TRUE;
}

/*! \brief   This function is used when Recv gets a MLME message
 *  \param  *Queue           The MLME Queue
 *  \param   TimeStampHigh   The upper 32 bit of timestamp
 *  \param   TimeStampLow    The lower 32 bit of timestamp
 *  \param   Rssi            The receiving RSSI strength
 *  \param   MsgLen          The length of the message
 *  \param  *Msg             The message pointer
 *  \return  TRUE if everything ok, FALSE otherwise (like Queue Full)
 *  \pre
 *  \post
 */
BOOLEAN MlmeEnqueueForRecv(
    IN	PRTMP_ADAPTER	pAd, 
    IN	PFRAME_802_11	p80211hdr,  // add by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
    IN ULONG TimeStampHigh, 
    IN ULONG TimeStampLow,
    IN UCHAR Rssi, 
    IN ULONG MsgLen, 
    IN VOID *Msg,
    IN UCHAR Signal)
{
    INT          Tail, Machine;
    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;
	PUCHAR	pFrame = (PUCHAR)Msg;
	// end johnli
    ULONG        MsgType;
	MLME_QUEUE	*Queue = (MLME_QUEUE *)&pAd->Mlme.Queue;
    unsigned long	IrqFlags;


    // Do nothing if the driver is starting halt state.
    // This might happen when timer already been fired before cancel timer with mlmehalt
    //if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
    //    return FALSE;

	// First check the size, it MUST not exceed the mlme queue size
	// edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
/*
	if (MsgLen > MAX_LEN_OF_MLME_BUFFER)
	{
        DBGPRINT_ERR("MlmeEnqueueForRecv: frame too large, size = %d \n", MsgLen);
		return FALSE;
	}
*/
	if (MsgLen + LENGTH_802_11 > MAX_LEN_OF_MLME_BUFFER)
	{
        DBGPRINT_ERR("MlmeEnqueueForRecv: frame too large, size = %d \n", MsgLen + LENGTH_802_11);
		return FALSE;
	}
	// end johnli

    if (MlmeQueueFull(Queue)) 
    {
        DBGPRINT_ERR("MlmeEnqueueForRecv: full and dropped\n");
        return FALSE;
    }

    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
/*
    if (!MsgTypeSubst(pAd, pFrame, &Machine, &MsgType)) 
    {
        DBGPRINT_ERR("MlmeEnqueueForRecv: un-recongnized mgmt->subtype=%d\n",pFrame->Hdr.FC.SubType);
        return FALSE;
    }
*/
    if (!MsgTypeSubst(pAd, p80211hdr, pFrame, &Machine, &MsgType)) 
    {
        DBGPRINT_ERR("MlmeEnqueueForRecv: un-recongnized mgmt->subtype=%d\n",p80211hdr->Hdr.FC.SubType);
        return FALSE;
    }
    // end johnli
    
    if ((Machine == SYNC_STATE_MACHINE)&&(MsgType == MT2_PEER_PROBE_RSP)&&(pAd->PortCfg.bGetAPConfig)) 
    {
        if (pAd->Mlme.SyncMachine.CurrState == SYNC_IDLE) {
            CHAR CfgData[MAX_CFG_BUFFER_LEN+1] = {0};
            if (BackDoorProbeRspSanity(pAd, Msg, MsgLen, CfgData)) {
                printk("MlmeEnqueueForRecv: CfgData(len:%d):\n%s\n", strlen(CfgData), CfgData);
                pAd->PortCfg.bGetAPConfig = FALSE;
            }
        }   
    }
    
    // OK, we got all the informations, it is time to put things into queue
	RTMP_SEM_LOCK(&(Queue->Lock), IrqFlags);

    Tail = Queue->Tail;
    Queue->Tail++;
    Queue->Num++;
    if (Queue->Tail == MAX_LEN_OF_MLME_QUEUE) 
    {
        Queue->Tail = 0;
    }


    DBGPRINT(RT_DEBUG_INFO, "MlmeEnqueueForRecv, num=%d\n",Queue->Num);
    
    Queue->Entry[Tail].Occupied = TRUE;
    Queue->Entry[Tail].Machine = Machine;
    Queue->Entry[Tail].MsgType = MsgType;
    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    Queue->Entry[Tail].MsgLen  = MsgLen;
    Queue->Entry[Tail].MsgLen  = MsgLen + LENGTH_802_11;
    // end johnli
    Queue->Entry[Tail].TimeStamp.vv.LowPart = TimeStampLow;
    Queue->Entry[Tail].TimeStamp.vv.HighPart = TimeStampHigh;
    Queue->Entry[Tail].Rssi = Rssi;
    Queue->Entry[Tail].Signal = Signal;
    Queue->Entry[Tail].Channel = pAd->LatchRfRegs.Channel;
    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    NdisMoveMemory(Queue->Entry[Tail].Msg, Msg, MsgLen);
    NdisMoveMemory(Queue->Entry[Tail].Msg, p80211hdr, LENGTH_802_11);
    NdisMoveMemory(&Queue->Entry[Tail].Msg[LENGTH_802_11], Msg, MsgLen);
    // end johnli


	RTMP_SEM_UNLOCK(&(Queue->Lock), IrqFlags);

    MlmeHandler(pAd);

    return TRUE;
}

/*! \brief   Dequeue a message from the MLME Queue
 *  \param  *Queue    The MLME Queue
 *  \param  *Elem     The message dequeued from MLME Queue
 *  \return  TRUE if the Elem contains something, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN MlmeDequeue(
    IN MLME_QUEUE *Queue, 
    OUT MLME_QUEUE_ELEM **Elem) 
{	
    unsigned long	IrqFlags;   

	RTMP_SEM_LOCK(&(Queue->Lock), IrqFlags);

    *Elem = &(Queue->Entry[Queue->Head]);
    Queue->Num--;
    Queue->Head++;
    if (Queue->Head == MAX_LEN_OF_MLME_QUEUE) 
    {
        Queue->Head = 0;
    }

	RTMP_SEM_UNLOCK(&(Queue->Lock), IrqFlags);

    DBGPRINT(RT_DEBUG_INFO, "MlmeDequeue, num=%d\n",Queue->Num);

    return TRUE;
}

VOID	MlmeRestartStateMachine(
    IN	PRTMP_ADAPTER	pAd)
{
    MLME_QUEUE_ELEM		*Elem = NULL;
    unsigned long		IrqFlags;  
	BOOLEAN				Cancelled;

	RTMP_SEM_LOCK(&pAd->Mlme.TaskLock, IrqFlags);

    if(pAd->Mlme.bRunning) 
    {
		RTMP_SEM_UNLOCK(&pAd->Mlme.TaskLock, IrqFlags);
        return;
    } 
    else 
    {
        pAd->Mlme.bRunning = TRUE;
    }

	RTMP_SEM_UNLOCK(&pAd->Mlme.TaskLock, IrqFlags);

	// Remove all Mlme queues elements
    while (!MlmeQueueEmpty(&pAd->Mlme.Queue)) 
    {
        //From message type, determine which state machine I should drive
        if (MlmeDequeue(&pAd->Mlme.Queue, &Elem)) 
        {
            // free MLME element
            Elem->Occupied = FALSE;
            Elem->MsgLen = 0;
            
        }
        else {
            DBGPRINT(RT_DEBUG_ERROR, "ERROR: empty Elem in MlmeQueue\n");
        }
    }

	// Cancel all timer events
	// Be careful to cancel new added timer
    RTMPCancelTimer(&pAd->MlmeAux.AssocTimer, &Cancelled);
    RTMPCancelTimer(&pAd->MlmeAux.ReassocTimer, &Cancelled);   
    RTMPCancelTimer(&pAd->MlmeAux.DisassocTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.AuthTimer, &Cancelled); 
    RTMPCancelTimer(&pAd->MlmeAux.BeaconTimer, &Cancelled);
    RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled);
    RTMPCancelTimer(&pAd->RfTuningTimer, &Cancelled);

	// Change back to original channel in case of doing scan
	AsicSwitchChannel(pAd, pAd->PortCfg.Channel);
	AsicLockChannel(pAd, pAd->PortCfg.Channel);

	// Resume MSDU which is turned off durning scan
	RTMPResumeMsduTransmission(pAd);

	// Set all state machines back IDLE
    pAd->Mlme.CntlMachine.CurrState    = CNTL_IDLE;
	pAd->Mlme.AssocMachine.CurrState   = ASSOC_IDLE;
	pAd->Mlme.AuthMachine.CurrState    = AUTH_REQ_IDLE;
	pAd->Mlme.AuthRspMachine.CurrState = AUTH_RSP_IDLE;
	pAd->Mlme.SyncMachine.CurrState    = SYNC_IDLE;
	
	// Remove running state
	RTMP_SEM_LOCK(&pAd->Mlme.TaskLock, IrqFlags);
    pAd->Mlme.bRunning = FALSE;
	RTMP_SEM_UNLOCK(&pAd->Mlme.TaskLock, IrqFlags);

}

/*! \brief  test if the MLME Queue is empty
 *  \param  *Queue    The MLME Queue
 *  \return TRUE if the Queue is empty, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN MlmeQueueEmpty(
    IN MLME_QUEUE *Queue) 
{
    BOOLEAN Ans;
    unsigned long	IrqFlags;

	RTMP_SEM_LOCK(&(Queue->Lock), IrqFlags);

    Ans = (Queue->Num == 0);

	RTMP_SEM_UNLOCK(&(Queue->Lock), IrqFlags);

    return Ans;
}

/*! \brief   test if the MLME Queue is full
 *  \param   *Queue      The MLME Queue
 *  \return  TRUE if the Queue is empty, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN MlmeQueueFull(
    IN MLME_QUEUE *Queue) 
{
    BOOLEAN Ans;
    unsigned long	IrqFlags;

	RTMP_SEM_LOCK(&(Queue->Lock), IrqFlags);

    Ans = (Queue->Num == MAX_LEN_OF_MLME_QUEUE);

	RTMP_SEM_UNLOCK(&(Queue->Lock), IrqFlags);

    return Ans;
}

/*! \brief   The destructor of MLME Queue
 *  \param 
 *  \return
 *  \pre
 *  \post
 *  \note   Clear Mlme Queue, Set Queue->Num to Zero.
 */
VOID MlmeQueueDestroy(
    IN MLME_QUEUE *pQueue) 
{
    unsigned long	IrqFlags; 

	RTMP_SEM_LOCK(&(pQueue->Lock), IrqFlags);

    pQueue->Num  = 0;
    pQueue->Head = 0;
    pQueue->Tail = 0;

	RTMP_SEM_UNLOCK(&(pQueue->Lock), IrqFlags);
    
    NdisFreeSpinLock(&(pQueue->Lock));
}

/*! \brief   To substitute the message type if the message is coming from external
 *  \param  pFrame         The frame received
 *  \param  *Machine       The state machine
 *  \param  *MsgType       the message type for the state machine
 *  \return TRUE if the substitution is successful, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN MsgTypeSubst(
	IN PRTMP_ADAPTER  pAd,
	// edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//	IN PFRAME_802_11 pFrame, 
	IN PFRAME_802_11 p80211hdr, 
	IN PUCHAR pFrame, 
	// end johnli
	OUT INT *Machine, 
	OUT INT *MsgType) 
{
	USHORT Seq;
	UCHAR 	EAPType; 
//	BOOLEAN     Return;  // remove by johnli

// only PROBE_REQ can be broadcast, all others must be unicast-to-me && is_mybssid; otherwise, 
// ignore this frame

	// wpa EAPOL PACKET
	// edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//	if (pFrame->Hdr.FC.Type == BTYPE_DATA) 
	if (p80211hdr->Hdr.FC.Type == BTYPE_DATA) 
	// end johnli
	{    
		*Machine = WPA_PSK_STATE_MACHINE;
		EAPType = *(pFrame+LENGTH_802_1_H+1);  // // add by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
        
		{
			// remove by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
/*
			EAPType = *((UCHAR*)pFrame+LENGTH_802_11+LENGTH_802_1_H+1);
			*Machine = WPA_PSK_STATE_MACHINE;
*/
			// end johnli
			return(WpaMsgTypeSubst(EAPType, MsgType));
		}
	}
    // edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    else if (pFrame->Hdr.FC.Type == BTYPE_MGMT) 		
    else if (p80211hdr->Hdr.FC.Type == BTYPE_MGMT)
    // end johnli
    {
    	// edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//    	switch (pFrame->Hdr.FC.SubType) 
    	switch (p80211hdr->Hdr.FC.SubType) 
    	// end johnli
    	{
			case SUBTYPE_ASSOC_REQ:
				*Machine = ASSOC_STATE_MACHINE;
				*MsgType = MT2_PEER_ASSOC_REQ;
				break;
            case SUBTYPE_ASSOC_RSP:
#if WPA_SUPPLICANT_SUPPORT
		COPY_MAC_ADDR(pAd->PortCfg.Bssid, p80211hdr->Hdr.Addr2);
#endif
                *Machine = ASSOC_STATE_MACHINE;
                *MsgType = MT2_PEER_ASSOC_RSP;
            break;    
	       	case SUBTYPE_REASSOC_REQ:
				*Machine = ASSOC_STATE_MACHINE;
				*MsgType = MT2_PEER_REASSOC_REQ;
				break;
            case SUBTYPE_REASSOC_RSP:
                *Machine = ASSOC_STATE_MACHINE;
                *MsgType = MT2_PEER_REASSOC_RSP;
            break;
            case SUBTYPE_PROBE_REQ:
                *Machine = SYNC_STATE_MACHINE;
                *MsgType = MT2_PEER_PROBE_REQ;
            break;
            case SUBTYPE_PROBE_RSP:
                *Machine = SYNC_STATE_MACHINE;
                *MsgType = MT2_PEER_PROBE_RSP;
            break;
        	case SUBTYPE_BEACON:
				*Machine = SYNC_STATE_MACHINE;
				*MsgType = MT2_PEER_BEACON;
            	break;
            case SUBTYPE_ATIM:
                *Machine = SYNC_STATE_MACHINE;
                *MsgType = MT2_PEER_ATIM;
            break;    
        	case SUBTYPE_DISASSOC:
				*Machine = ASSOC_STATE_MACHINE;
				*MsgType = MT2_PEER_DISASSOC_REQ;
				break;
        	case SUBTYPE_AUTH:
	            // get the sequence number from payload 24 Mac Header + 2 bytes algorithm
				// edit by johnli, fix WPS & WPAPSK/WPA2PSK bugs for receiving EAPoL fragmentation packets
//				NdisMoveMemory(&Seq, &pFrame->Octet[2], sizeof(USHORT));
				NdisMoveMemory(&Seq, &pFrame[2], sizeof(USHORT));
				// end johnli
				if (Seq == 1 || Seq == 3) 
                {
                    *Machine = AUTH_RSP_STATE_MACHINE;
                    *MsgType = MT2_PEER_AUTH_ODD;
                } 
                else if (Seq == 2 || Seq == 4) 
                {
                    *Machine = AUTH_STATE_MACHINE;
                    *MsgType = MT2_PEER_AUTH_EVEN;
                } 
                else 
                {
                    return FALSE;
                }
				break;
        	case SUBTYPE_DEAUTH:
				*Machine = AUTH_RSP_STATE_MACHINE;
				*MsgType = MT2_PEER_DEAUTH;
				break;
        	default:
				return FALSE;
    	}

   		return TRUE;
    }

    return FALSE;
}

// ===========================================================================================
// state_machine.c
// ===========================================================================================

/*! \brief Initialize the state machine.
 *  \param *S           pointer to the state machine 
 *  \param  Trans       State machine transition function
 *  \param  StNr        number of states 
 *  \param  MsgNr       number of messages 
 *  \param  DefFunc     default function, when there is invalid state/message combination 
 *  \param  InitState   initial state of the state machine 
 *  \param  Base        StateMachine base, internal use only
 *  \pre p_sm should be a legal pointer
 *  \post
 */
VOID StateMachineInit(
    IN STATE_MACHINE *S, 
    IN STATE_MACHINE_FUNC Trans[], 
    IN ULONG StNr, 
    IN ULONG MsgNr, 
    IN STATE_MACHINE_FUNC DefFunc, 
    IN ULONG InitState, 
    IN ULONG Base) 
{
    ULONG i, j;

    // set number of states and messages
    S->NrState = StNr;
    S->NrMsg   = MsgNr;
    S->Base    = Base;

    S->TransFunc  = Trans;
    
    // init all state transition to default function
    for (i = 0; i < StNr; i++) 
    {
        for (j = 0; j < MsgNr; j++) 
        {
            S->TransFunc[i * MsgNr + j] = DefFunc;
        }
    }
    
    // set the starting state
    S->CurrState = InitState;

}

/*! \brief This function fills in the function pointer into the cell in the state machine 
 *  \param *S   pointer to the state machine
 *  \param St   state
 *  \param Msg  incoming message
 *  \param f    the function to be executed when (state, message) combination occurs at the state machine
 *  \pre *S should be a legal pointer to the state machine, st, msg, should be all within the range, Base should be set in the initial state
 *  \post
 */
VOID StateMachineSetAction(
    IN STATE_MACHINE *S, 
    IN ULONG St, 
    IN ULONG Msg, 
    IN STATE_MACHINE_FUNC Func) 
{
    ULONG MsgIdx;
    
    MsgIdx = Msg - S->Base;

    if (St < S->NrState && MsgIdx < S->NrMsg) 
    {
        // boundary checking before setting the action
        S->TransFunc[St * S->NrMsg + MsgIdx] = Func;
    } 
}

/*! \brief   This function does the state transition
 *  \param   *Adapter the NIC adapter pointer
 *  \param   *S       the state machine
 *  \param   *Elem    the message to be executed
 *  \return   None
 */
VOID StateMachinePerformAction(
    IN	PRTMP_ADAPTER	pAd, 
    IN STATE_MACHINE *S, 
    IN MLME_QUEUE_ELEM *Elem) 
{
    (*(S->TransFunc[S->CurrState * S->NrMsg + Elem->MsgType - S->Base]))(pAd, Elem);
}

/*
    ==========================================================================
    Description:
        The drop function, when machine executes this, the message is simply 
        ignored. This function does nothing, the message is freed in 
        StateMachinePerformAction()
    ==========================================================================
 */
VOID Drop(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
}

// ===========================================================================================
// lfsr.c
// ===========================================================================================

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID LfsrInit(
    IN PRTMP_ADAPTER pAd, 
    IN unsigned long Seed) 
{
    if (Seed == 0) 
        pAd->Mlme.ShiftReg = 1;
    else 
        pAd->Mlme.ShiftReg = Seed;
}

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
UCHAR RandomByte(
    IN PRTMP_ADAPTER pAd) 
{
    ULONG i;
    UCHAR R, Result;

    R = 0;

    for (i = 0; i < 8; i++) 
    {
        if (pAd->Mlme.ShiftReg & 0x00000001) 
        {
            pAd->Mlme.ShiftReg = ((pAd->Mlme.ShiftReg ^ LFSR_MASK) >> 1) | 0x80000000;
            Result = 1;
        } 
        else 
        {
            pAd->Mlme.ShiftReg = pAd->Mlme.ShiftReg >> 1;
            Result = 0;
        }
        R = (R << 1) | Result;
    }

    return R;
}

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID AsicSwitchChannel(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR Channel) 
{
	ULONG R3 = DEFAULT_RF_TX_POWER, R4;
	CHAR TxPwer = 0, Bbp94 = BBPR94_DEFAULT;
    UCHAR index, BbpReg;
    RTMP_RF_REGS *RFRegTable;

    // Select antenna
    AsicAntennaSelect(pAd, Channel);
        
	// Search Tx power value
	for (index = 0; index < pAd->ChannelListNum; index++)
	{
		if (Channel == pAd->ChannelList[index].Channel)
		{
			TxPwer = pAd->ChannelList[index].Power;
			break;
		}
	}
	
	if (TxPwer > 31)  
	{
		//
		// R3 can't large than 36 (0x24), 31 ~ 36 used by BBP 94
		//		
		R3 = 31;
		if (TxPwer <= 36)
			Bbp94 = BBPR94_DEFAULT + (UCHAR) (TxPwer - 31);		
	}
	else if (TxPwer < 0)
	{
		//
		// R3 can't less than 0, -1 ~ -6 used by BBP 94
		//	
		R3 = 0;
		if (TxPwer >= -6)
			Bbp94 = BBPR94_DEFAULT + TxPwer;
	}
	else
	{  
		// 0 ~ 31
		R3 = (ULONG) TxPwer;
	}
	
#if 1
	// E2PROM setting is calibrated for maximum TX power (i.e. 100%)
	// We lower TX power here according to the percentage specified from UI
	if (pAd->PortCfg.TxPowerPercentage > 90)       // 91 ~ 100%, treat as 100% in terms of mW
		;
	else if (pAd->PortCfg.TxPowerPercentage > 60)  // 61 ~ 90%, treat as 75% in terms of mW    
	{
		if (R3 > 2)
			R3 -= 2;
		else 
			R3 = 0;
	}
	else if (pAd->PortCfg.TxPowerPercentage > 30)  // 31 ~ 60%, treat as 50% in terms of mW
	{
		if (R3 > 6)
			R3 -= 6;
		else 
			R3 = 0;
	}
	else if (pAd->PortCfg.TxPowerPercentage > 15)  // 16 ~ 30%, treat as 25% in terms of mW
	{
		if (R3 > 12)
			R3 -= 12;
		else 
			R3 = 0;
	}
	else if (pAd->PortCfg.TxPowerPercentage > 9)   // 10 ~ 15%, treat as 12.5% in terms of mW
	{
		if (R3 > 18)
			R3 -= 18;
		else 
			R3 = 0;
	}
	else                                             // 0 ~ 9 %, treat as 6.25% in terms of mW
	{
		if (R3 > 24)
			R3 -= 24;
		else 
			R3 = 0;
	}
  
	if (R3 > 31)  R3 = 31;	// Maximum value 31
  
	if (Bbp94 < 0) Bbp94 = 0;

	R3 = R3 << 9; // shift TX power control to correct RF R3 bit position
#endif
    
	if (pAd->RFProgSeq == 0)
		RFRegTable = RF5225RegTable;
	else
		RFRegTable = RF5225RegTable_1;
    
    switch (pAd->RfIcType)
    {
        case RFIC_5225:
        case RFIC_5325:
        case RFIC_2527:
		case RFIC_2529:

            for (index = 0; index < NUM_OF_5225_CHNL; index++)
            {
                if (Channel == RFRegTable[index].Channel)
                {
                    R3 = R3 | (RFRegTable[index].R3 & 0xffffc1ff); // set TX power
                    R4 = (RFRegTable[index].R4 & (~0x0003f000)) | (pAd->RfFreqOffset << 12);

                    // Update variables
                    pAd->LatchRfRegs.Channel = Channel;
                    pAd->LatchRfRegs.R1 = RFRegTable[index].R1;
                    pAd->LatchRfRegs.R2 = RFRegTable[index].R2;
                    pAd->LatchRfRegs.R3 = R3;
                    pAd->LatchRfRegs.R4 = R4;

			// Set RF value 1's set R3[bit2] = [0]
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
			RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

			RTMPusecDelay(200);

			// Set RF value 2's set R3[bit2] = [1]
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
			RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 | 0x04));
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

			RTMPusecDelay(200);

			// Set RF value 3's set R3[bit2] = [0]
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
			RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
			RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

                    break;
                }
            }
            RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BbpReg);
            if ((pAd->RfIcType == RFIC_5225) || (pAd->RfIcType == RFIC_2527))
                BbpReg &= 0xFE;    // b0=0 for none smart mode
            else
                BbpReg |= 0x01;    // b0=1 for smart mode
            RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BbpReg);

			if (Bbp94 != BBPR94_DEFAULT)
			{
                RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R94, Bbp94);
				pAd->Bbp94 = Bbp94;
			}
            break;

        default:
            break;
    }

	if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		if (Channel <= 14)
		{
			if (pAd->BbpTuning.R17LowerUpperSelect == 0)
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->BbpTuning.R17LowerBoundG);
			}
			else
			{	
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->BbpTuning.R17UpperBoundG);
			}
		}
		else
		{
			if (pAd->BbpTuning.R17LowerUpperSelect == 0)
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->BbpTuning.R17LowerBoundA);
			}
			else
			{
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->BbpTuning.R17UpperBoundA);
			}
		}
	}

	//
	// On 11A, We should delay and wait RF/BBP to be stable
	// and the appropriate time should be 1000 micro seconds 
	//
    RTMPusecDelay(1000);
    
    DBGPRINT(RT_DEBUG_TRACE, "AsicSwitchChannel(RF=%d, Pwr=%d) to #%d, R1=0x%08x, R2=0x%08x, R3=0x%08x, R4=0x%08x\n",
        pAd->RfIcType, 
        (R3 & 0x00003e00) >> 9,
        Channel, 
        pAd->LatchRfRegs.R1, 
        pAd->LatchRfRegs.R2, 
        pAd->LatchRfRegs.R3, 
        pAd->LatchRfRegs.R4);
}

/*
    ==========================================================================
    Description:
        This function is required for 2421 only, and should not be used during
        site survey. It's only required after NIC decided to stay at a channel
        for a longer period.
        When this function is called, it's always after AsicSwitchChannel().
    ==========================================================================
 */
VOID AsicLockChannel(
    IN PRTMP_ADAPTER pAd, 
    IN UCHAR Channel) 
{
}


char *AntStr[4] = {"SW Diversity","Ant-A","Ant-B","HW Diversity"};

#define SOFTWARE_DIVERSITY  0
#define ANTENNA_A		    1
#define ANTENNA_B		    2
#define HARDWARE_DIVERSITY  3

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID	AsicAntennaSelect(
    IN	PRTMP_ADAPTER	pAd,
    IN	UCHAR			Channel) 
{
	ULONG			csr0;	
	ABGBAND_STATE	BandState;

	DBGPRINT(RT_DEBUG_INFO,"AsicAntennaSelect(ch=%d) - Tx=%s, Rx=%s\n", 
	    Channel, AntStr[pAd->Antenna.field.TxDefaultAntenna], AntStr[pAd->Antenna.field.RxDefaultAntenna]);
	    
	if (Channel <= 14)
		BandState = BG_BAND;
	else
		BandState = A_BAND;

	//
	//  Only the first time switch from g to a or a to g
	//  and then will be reset the BBP, otherwise do nothing.
	//
	if (BandState == pAd->PortCfg.BandState)
		return;

	// Change BBP setting during siwtch from a->g, g->a
    if (Channel <= 14)
    {
		if (pAd->NicConfig2.field.ExternalLNAForG)
    	{
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, 0x30); // if external LNA enable, this value need to be offset 0x10
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 96, 0x68); // if external LNA enable, R96 need to shit 0x20 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 104, 0x3C);// if external LNA enable, R104 need to shit 0x10 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 75, 0x80);// if external LNA enable, set this to 0x80 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 86, 0x80);// if external LNA enable, set this to 0x80 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 88, 0x80);// if external LNA enable, set this to 0x80 on B/G mode, Request by David 2005/05/12 
    	}
    	else
    	{
        	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, 0x20);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 96, 0x48);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 104, 0x2C);
    	}
        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 35, 0x50);        
        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 97, 0x48);
        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 98, 0x48);
    }
    else
    {
		if (pAd->NicConfig2.field.ExternalLNAForA)
    	{
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, 0x38); // if external LNA enable, this value need to be offset 0x10
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 96, 0x78); // if external LNA enable, R96 need to shit 0x20 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 104, 0x48);// if external LNA enable, R104 need to shit 0x10 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 75, 0x80);// if external LNA enable, set this to 0x80 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 86, 0x80);// if external LNA enable, set this to 0x80 on B/G mode, Request by David 2005/05/12 
    		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 88, 0x80);// if external LNA enable, set this to 0x80 on B/G mode, Request by David 2005/05/12     		
    	}
    	else
    	{
        	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, 0x28);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 96, 0x58);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 104, 0x38);
    	}
        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 35, 0x60);        
        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 97, 0x58);
        RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 98, 0x58);
    }
    
    // Turn on PA_PE for 11A / 11G band
    RTMP_IO_READ32(pAd, PHY_CSR0, &csr0);
    csr0 = csr0 & 0xfffcffff;   // Mask off bit 16, bit 17
    if (Channel <= 14)
        csr0 = csr0 | BIT32[16]; // b16 to enable G band PA_PE
    else
        csr0 = csr0 | BIT32[17]; // b17 to enable A band PA_PE
    RTMP_IO_WRITE32(pAd, PHY_CSR0, csr0);

	pAd->PortCfg.BandState = BandState;

	AsicAntennaSetting(pAd, BandState);
}

/*
	========================================================================
	
	Routine Description:
		Antenna miscellaneous setting.

	Arguments:
		pAd						Pointer to our adapter
		BandState				Indicate current Band State.

	Return Value:
		None
	
	Note:
		1.) Frame End type control
			only valid for G only (RF_2527 & RF_2529)
			0: means DPDT, set BBP R4 bit 5 to 1
			1: means SPDT, set BBP R4 bit 5 to 0
			

	========================================================================
*/
VOID	AsicAntennaSetting(
	IN	PRTMP_ADAPTER	pAd,
	IN	ABGBAND_STATE	BandState)
{
	UCHAR		R3 = 0, R4 = 0, R77 = 0;
	UCHAR		FrameTypeMaskBit5 = 0;
	
	// driver must disable Rx when switching antenna, otherwise ASIC will keep default state
	// after switching, driver needs to re-enable Rx later
	RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);        
	
	// Update antenna registers
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &R3);
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &R4);
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R77, &R77);

	R3  &= 0xfe;		// clear Bit 0
	R4	&= ~0x23;		// clear Bit 0,1,5

	FrameTypeMaskBit5 = ~(pAd->Antenna.field.FrameType << 5);
	

	if ((pAd->Antenna.field.RfIcType == RFIC_5325) || 
		(pAd->Antenna.field.RfIcType == RFIC_2529))
	{
		R3 |= 0x01;  //RFIC_5325, RFIC_2529, <Bit0> = <1>
	}

	//Select RF_Type
	switch (pAd->Antenna.field.RfIcType)
	{
		case RFIC_5225:
		case RFIC_5325:
			//Support 11B/G/A			
			if (BandState == BG_BAND)
			{
				//Check Rx Anttena
				if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_A)
				{
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					R77 = R77 | 0x03;	    // <Bit1:Bit0> = <1:1>
					
					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
				}
				else if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_B)
				{
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					R77 = R77 & 0xfc;		// <Bit1:Bit0> = <0:0>

					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
				}
				else if (pAd->Antenna.field.RxDefaultAntenna == HARDWARE_DIVERSITY)
				{
					R4  = R4 | 0x22;		// <Bit5:Bit1:Bit0> = <1:1:0>
				}
				else 
				{
					; //SOFTWARE_DIVERSITY
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					pAd->RxAnt.Pair1PrimaryRxAnt   = 1;  // assume ant-B
					pAd->RxAnt.Pair1SecondaryRxAnt = 0;  // assume ant-A
					
					AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
				}
			}
			else //A_BAND
			{
				//Check Rx Anttena
				if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_A)
				{
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					R77 = R77 & 0xfc;		// <Bit1:Bit0> = <0:0>

					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
				}
				else if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_B)
				{
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					R77	= R77 | 0x03;	    // <Bit1:Bit0> = <1:1>

					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
				}
				else if (pAd->Antenna.field.RxDefaultAntenna == HARDWARE_DIVERSITY)
				{
					R4  = R4 | 0x02;		// <Bit5:Bit1:Bit0> = <0:1:0>
				}
				else 
				{
					; //SOFTWARE_DIVERSITY
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					pAd->RxAnt.Pair1PrimaryRxAnt   = 1;  // assume ant-B
					pAd->RxAnt.Pair1SecondaryRxAnt = 0;  // assume ant-A
					AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
				}
			}
			break;
		case RFIC_2527:
			//Support 11B/G
			//Check Rx Anttena
			//Check Rx Anttena
			if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_A)
			{
				R4  = R4 | 0x21;		// <Bit5:Bit1:Bit0> = <1:0:1>
				R4  = R4 & FrameTypeMaskBit5;
				
				R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>

				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
			}
			else if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_B)
			{
				R4  = R4 | 0x21;		// <Bit5:Bit1:Bit0> = <1:0:1>
				R4  = R4 & FrameTypeMaskBit5;
				
				R77 = R77 & 0xfc;		// <Bit1:Bit0> = <0:0>

				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
			}
			else if (pAd->Antenna.field.RxDefaultAntenna == HARDWARE_DIVERSITY)
			{
				R4  = R4 | 0x22;		// <Bit5:Bit1:Bit0> = <1:1:0>
				R4  = R4 & FrameTypeMaskBit5;
			}
			else 
			{
				; //SOFTWARE_DIVERSITY
				R4  = R4 | 0x21;		// <Bit5:Bit1:Bit0> = <1:0:1>
				R4  = R4 & FrameTypeMaskBit5;
				
				pAd->RxAnt.Pair1PrimaryRxAnt   = 1;  // assume ant-B
				pAd->RxAnt.Pair1SecondaryRxAnt = 0;  // assume ant-A
				AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
			}
			break;			
		case RFIC_2529:			
			//Support 11B/G
			//Check Rx Anttena
			if (pAd->Antenna.field.NumOfAntenna == 0)
			{
				if ((pAd->NicConfig2.field.Enable4AntDiversity == 1)
					&& (pAd->NicConfig2.field.TxDiversity == 1))// Tx/Rx diversity
				{
					//R77;				// <Bit1:Bit0>
					R4  = R4 | 0x22;		// <Bit5:Bit1:Bit0> = <1:1:0>

					pAd->RxAnt.Pair1PrimaryRxAnt   = 0;  // assume ant-B
					pAd->RxAnt.Pair1SecondaryRxAnt = 1;  // assume ant-A
					pAd->RxAnt.Pair2PrimaryRxAnt   = 1;  // assume ant-B
					pAd->RxAnt.Pair2SecondaryRxAnt = 0;  // assume ant-A
					pAd->RxAnt.Pair1AvgRssi[0] = (-95) << 3;  // reset Ant-A's RSSI history
					pAd->RxAnt.Pair1AvgRssi[1] = (-95) << 3;  // reset Ant-B's RSSI history
					pAd->RxAnt.Pair2AvgRssi[0] = (-95) << 3;  // reset Ant-A's RSSI history
					pAd->RxAnt.Pair2AvgRssi[1] = (-95) << 3;  // reset Ant-B's RSSI history
					AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
				}
				else if ((pAd->NicConfig2.field.Enable4AntDiversity == 1) &&
					(pAd->NicConfig2.field.TxDiversity == 0)	)		// Tx fix & Rx diversity
				{
					if (pAd->NicConfig2.field.TxRxFixed == 0)			// TxRxFixed <Tx:Rx> = <E1/E1:E4>
					{
						//R77;				// <Bit1:Bit0>
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 1)			// TxRxFixed <Tx:Rx> = <E2/E2:E3>
					{
						//R77;				// <Bit1:Bit0>
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 2)			// TxRxFixed <Tx:Rx> = <E3/E1:E3>
					{
						R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
						RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 3)			// TxRxFixed <Tx:Rx> = <E4/E2:E4>
					{
						R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
						RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
					}
					
					R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
					
					pAd->RxAnt.Pair1PrimaryRxAnt   = 1;  // assume ant-B
					pAd->RxAnt.Pair1SecondaryRxAnt = 0;  // assume ant-A
					pAd->RxAnt.Pair2PrimaryRxAnt   = 1;  // assume ant-B
					pAd->RxAnt.Pair2SecondaryRxAnt = 0;  // assume ant-A
					pAd->RxAnt.Pair1AvgRssi[0] = (-95) << 3;  // reset Ant-A's RSSI history
					pAd->RxAnt.Pair1AvgRssi[1] = (-95) << 3;  // reset Ant-B's RSSI history
					pAd->RxAnt.Pair2AvgRssi[0] = (-95) << 3;  // reset Ant-A's RSSI history
					pAd->RxAnt.Pair2AvgRssi[1] = (-95) << 3;  // reset Ant-B's RSSI history
					AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
				}
				else if ((pAd->NicConfig2.field.Enable4AntDiversity == 0) &&
					(pAd->NicConfig2.field.TxDiversity == 1)	)		// Tx diversity & 2R
				{
					R4  = R4 | 0x02;		// <Bit5:Bit1:Bit0> = <0:1:0>
					
					//R77;				// <Bit1:Bit0>
					
					if (pAd->NicConfig2.field.TxRxFixed == 0)			// TxRxFixed <Tx:Rx> = <E1/E1:E4>
					{
						AsicSetRxAnt(pAd, 0, 1);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 1)			// TxRxFixed <Tx:Rx> = <E2/E2:E3>
					{
						AsicSetRxAnt(pAd, 1, 0);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 2)			// TxRxFixed <Tx:Rx> = <E3/E1:E3>
					{
						AsicSetRxAnt(pAd, 0, 0);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 3)			// TxRxFixed <Tx:Rx> = <E4/E2:E4>
					{
						AsicSetRxAnt(pAd, 1, 1);
					}
				}
				else if ((pAd->NicConfig2.field.Enable4AntDiversity == 0) &&
					(pAd->NicConfig2.field.TxDiversity == 0)	)		// Tx fix & 2R
				{
					if (pAd->NicConfig2.field.TxRxFixed == 0)			// TxRxFixed <Tx:Rx> = <E1/E1:E4>
					{
						R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>						
						
						R77 = R77 & 0xfc;				// <Bit1:Bit0> = <0:0>						
						RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
						AsicSetRxAnt(pAd, 0, 1);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 1)			// TxRxFixed <Tx:Rx> = <E2/E2:E3>
					{
						R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>						
						
						R77 = R77 & 0xfc;				// <Bit1:Bit0> = <0:0>
						RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
						AsicSetRxAnt(pAd, 1, 0);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 2)			// TxRxFixed <Tx:Rx> = <E3/E1:E3>
					{
						R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>						
						
						R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
						RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
						AsicSetRxAnt(pAd, 0, 0);
					}
					else if (pAd->NicConfig2.field.TxRxFixed == 3)			// TxRxFixed <Tx:Rx> = <E4/E2:E4>
					{
						R4  = R4 | 0x01;		// <Bit5:Bit1:Bit0> = <0:0:1>
						
						R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
						RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);
						AsicSetRxAnt(pAd, 1, 1);
					}
				}
			}
			else if (pAd->Antenna.field.NumOfAntenna == 2)
			{
				if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_A)
				{
					R4  = R4 | 0x21;		// <Bit5:Bit1:Bit0> = <1:0:1>
					R4  = R4 & FrameTypeMaskBit5;
					
					R77	= R77 | 0x03;	    // <Bit1:Bit0> = <1:1>

					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);					
				}
				else if (pAd->Antenna.field.RxDefaultAntenna == ANTENNA_B)
				{
					R4  = R4 | 0x21;		// <Bit5:Bit1:Bit0> = <1:0:1>
					R4  = R4 & FrameTypeMaskBit5;

					R77 = R77 & 0xfc;		// <Bit1:Bit0> = <0:0>

					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);		
				}
				else if (pAd->Antenna.field.RxDefaultAntenna == HARDWARE_DIVERSITY)
				{
					R4  = R4 | 0x22;		// <Bit5:Bit1:Bit0> = <1:0:1>
					R4  = R4 & FrameTypeMaskBit5;
				}
				else
				{
					//SOFTWARE_DIVERSITY
					R4  = R4 | 0x21;		// <Bit5:Bit1:Bit0> = <1:0:1>
					R4  = R4 & FrameTypeMaskBit5;
					
					pAd->RxAnt.Pair1PrimaryRxAnt   = 1;  // assume ant-B
					pAd->RxAnt.Pair1SecondaryRxAnt = 0;  // assume ant-A
					AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
				}
			}
			break;
		default:
			DBGPRINT(RT_DEBUG_TRACE, "Unkown RFIC Type\n");
			break;
	}

	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, R3);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, R4);
	

    RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x025eb032);       // Staion not drop control frame will fail WiFi Certification.

}

VOID AsicRfTuningExec(
    IN	unsigned long data) 
{
    RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)data;
	
	// Do nothing if the driver is starting halt state.
	// This might happen when timer already been fired before cancel timer with mlmehalt
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		return;
	
    switch (pAd->RfIcType)
    {
        case RFIC_5225:
        case RFIC_5325:
            pAd->LatchRfRegs.R1 &= 0xfffdffff;  // RF R1.bit17 "tune_en1" OFF
            pAd->LatchRfRegs.R3 &= 0xfffffeff;   // RF R3.bit8 "tune_en2" OFF
            RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1); 
            RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R3); 
//          DBGPRINT(RT_DEBUG_TRACE, "AsicRfTuningExec(R1=0x%x,R3=0x%x)\n",pAd->LatchRfRegs.R1,pAd->LatchRfRegs.R3);
            break;
            
        default:
            break;
    }
}

/*
    ==========================================================================
    Description:
        Gives CCK TX rate 2 more dB TX power.
        This routine works only in LINK UP in INFRASTRUCTURE mode.

        calculate desired Tx power in RF R3.Tx0~5,  should consider -
        0. if current radio is a noisy environment (pAd->DrsCounters.fNoisyEnvironment)
        1. TxPowerPercentage
        2. auto calibration based on TSSI feedback
        3. extra 2 db for CCK
        4. -10 db upon very-short distance (AvgRSSI >= -40db) to AP

    NOTE: Since this routine requires the value of (pAd->DrsCounters.fNoisyEnvironment),
        it should be called AFTER MlmeDynamicTxRatSwitching()
    ==========================================================================
 */
VOID AsicAdjustTxPower(
    IN PRTMP_ADAPTER pAd) 
{
    ULONG   R3, CurrTxPwr;
    SHORT   dbm;
    UCHAR   TxRate, Channel, index;
    BOOLEAN bAutoTxAgc = FALSE;
    UCHAR   TssiRef, *pTssiMinusBoundary, *pTssiPlusBoundary, TxAgcStep;
    UCHAR   BbpR1, idx;
	PCHAR	pTxAgcCompensate;
	CHAR	TxPwer=0;

    TxRate  = pAd->PortCfg.TxRate;
    Channel = pAd->PortCfg.Channel;
    
    dbm = pAd->PortCfg.AvgRssi - pAd->BbpRssiToDbmDelta;

 	if ((pAd->RfIcType == RFIC_5325) || (pAd->RfIcType == RFIC_2529))
	{
		if (pAd->PortCfg.AvgRssi2 > pAd->PortCfg.AvgRssi)
		{
			dbm = pAd->PortCfg.AvgRssi2 - pAd->BbpRssiToDbmDelta;
		}
	}

    
    // get TX Power base from E2PROM
    R3 = DEFAULT_RF_TX_POWER;
    for (index= 0 ; index < pAd->ChannelListNum; index++)
    {
        if (pAd->ChannelList[index].Channel == pAd->PortCfg.Channel)
        {
			TxPwer = pAd->ChannelList[index].Power;
            break;
        }
    }

    
    //
    // Correct R3 value, R3 value should be in range 0 ~ 31.
    //
    if (TxPwer < 0)
        R3 = 0;
    else if (TxPwer > 31)
        R3 = 31;
    else
        R3 = (ULONG) TxPwer;


    // error handling just in case
    if (index >= pAd->ChannelListNum)
    {
        DBGPRINT_ERR("AsicAdjustTxPower(can find pAd->PortCfg.Channel=%d in ChannelList[%d]\n", pAd->PortCfg.Channel, pAd->ChannelListNum);
        return;
    }

    // TX power compensation for temperature variation based on TSSI. try every 4 second
    if (pAd->Mlme.PeriodicRound % 4 == 0)
    {
        if (pAd->PortCfg.Channel <= 14)
        {
            bAutoTxAgc         = pAd->bAutoTxAgcG;
            TssiRef            = pAd->TssiRefG;
            pTssiMinusBoundary = &pAd->TssiMinusBoundaryG[0];
            pTssiPlusBoundary  = &pAd->TssiPlusBoundaryG[0];
            TxAgcStep          = pAd->TxAgcStepG;
			pTxAgcCompensate   = &pAd->TxAgcCompensateG;
        }
        else
        {
            bAutoTxAgc         = pAd->bAutoTxAgcA;
            TssiRef            = pAd->TssiRefA;
            pTssiMinusBoundary = &pAd->TssiMinusBoundaryA[0];
            pTssiPlusBoundary  = &pAd->TssiPlusBoundaryA[0];
            TxAgcStep          = pAd->TxAgcStepA;
			pTxAgcCompensate   = &pAd->TxAgcCompensateA;
        }

        if (bAutoTxAgc)
        {
            RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1,  &BbpR1);
            if (BbpR1 > pTssiMinusBoundary[1])
            {
                // Reading is larger than the reference value
                // check for how large we need to decrease the Tx power
                for (idx = 1; idx < 5; idx++)
                {
                    if (BbpR1 <= pAd->TssiMinusBoundaryG[idx])  // Found the range
                        break;
                }
                // The index is the step we should decrease, idx = 0 means there is nothing to compensate
                if (R3 > (ULONG) (TxAgcStep * (idx-1)))
                    *pTxAgcCompensate = -(TxAgcStep * (idx-1));
                else
                    *pTxAgcCompensate = -((UCHAR)R3);
				
                R3 += (*pTxAgcCompensate);
                DBGPRINT(RT_DEBUG_TRACE, "-- Tx Power, BBP R1=%x, TssiRef=%x, TxAgcStep=%x, step = -%d\n",
                    BbpR1, TssiRef, TxAgcStep, idx-1);                    
            }
            else if (BbpR1 < pTssiPlusBoundary[1])
            {
                // Reading is smaller than the reference value
                // check for how large we need to increase the Tx power
                for (idx = 1; idx < 5; idx++)
                {
                    if (BbpR1 >= pTssiPlusBoundary[idx])   // Found the range
                        break;
                }
                // The index is the step we should increase, idx = 0 means there is nothing to compensate
                *pTxAgcCompensate = TxAgcStep * (idx-1);
                R3 += (*pTxAgcCompensate);
                DBGPRINT(RT_DEBUG_TRACE, "++ Tx Power, BBP R1=%x, Tssi0=%x, TxAgcStep=%x, step = +%d\n",
                    BbpR1, TssiRef, TxAgcStep, idx-1);
            }
        }
    }
	else
	{
		if (pAd->PortCfg.Channel <= 14)
		{
            bAutoTxAgc         = pAd->bAutoTxAgcG;
			pTxAgcCompensate   = &pAd->TxAgcCompensateG;
		}
		else
		{
            bAutoTxAgc         = pAd->bAutoTxAgcA;
			pTxAgcCompensate   = &pAd->TxAgcCompensateA;
		}

		if (bAutoTxAgc)
			R3 += (*pTxAgcCompensate);
	}


	// E2PROM setting is calibrated for maximum TX power (i.e. 100%)
	// We lower TX power here according to the percentage specified from UI
	if (pAd->PortCfg.TxPowerPercentage == 0xffffffff)       // AUTO TX POWER control
	{
#if 1
        // only INFRASTRUCTURE mode and AUTO-TX-power need furthur calibration
        if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
        {
            // low TX power upon very-short distance to AP to solve some vendor's AP RX problem
            // in this case, no TSSI compensation is required.
            if (dbm > RSSI_FOR_LOWEST_TX_POWER)
            { 
                if (R3 > LOWEST_TX_POWER_DELTA)
                    R3 -= LOWEST_TX_POWER_DELTA;
                else 
                    R3 = 0;
            }
            else if (dbm > RSSI_FOR_LOW_TX_POWER)
            {
                if (R3 > LOW_TX_POWER_DELTA)
                    R3 -= LOW_TX_POWER_DELTA;
                else 
                    R3 = 0;
            }
        }
#endif
	}
	else if (pAd->PortCfg.TxPowerPercentage > 90)  // 91 ~ 100% & AUTO, treat as 100% in terms of mW
		;
	else if (pAd->PortCfg.TxPowerPercentage > 60)  // 61 ~ 90%, treat as 75% in terms of mW
	{
		if (R3 > 2)
			R3 -= 2;
		else
			R3 = 0;
    }
	else if (pAd->PortCfg.TxPowerPercentage > 30)  // 31 ~ 60%, treat as 50% in terms of mW
	{
		if (R3 > 6)			
			R3 -= 6;
		else
			R3 = 0;
	}
	else if (pAd->PortCfg.TxPowerPercentage > 15)  // 16 ~ 30%, treat as 25% in terms of mW
	{
		if (R3 > 12)
			R3 -= 12;
		else
			R3 = 0;
	}
	else if (pAd->PortCfg.TxPowerPercentage > 9)   // 10 ~ 15%, treat as 12.5% in terms of mW
	{
		if (R3 > 18)
			R3 -= 18;
		else
			R3 = 0;
	}
	else                                           // 0 ~ 9 %, treat as MIN(~3%) in terms of mW
	{
		if (R3 > 24)
			R3 -= 24;
		else
			R3 = 0;
	}

	if (R3 > 31)  R3 = 31;   //Maximum value 31

    // compare the desired R3.TxPwr value with current R3, if not equal
    // set new R3.TxPwr
    CurrTxPwr = (pAd->LatchRfRegs.R3 >> 9) & 0x0000001f;
    if (CurrTxPwr != R3)
    {
		CurrTxPwr = R3;
		R3 = (pAd->LatchRfRegs.R3 & 0xffffc1ff) | (R3 << 9);
		pAd->LatchRfRegs.R3 = R3;

		// Set RF value 1's set R3[bit2] = [0]
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
		RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

		RTMPusecDelay(200);

		// Set RF value 2's set R3[bit2] = [1]
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
		RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 | 0x04));
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

		RTMPusecDelay(200);
		
		// Set RF value 3's set R3[bit2] = [0]
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
		RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
		RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);
		
    }
        DBGPRINT(RT_DEBUG_TRACE, "AsicAdjustTxPower = %d, AvgRssi = %d\n", CurrTxPwr, dbm);
    
}

/*
    ==========================================================================
    Description:
        put PHY to sleep here, and set next wakeup timer. PHY doesn't not wakeup 
        automatically. Instead, MCU will issue a TwakeUpInterrupt to host after
        the wakeup timer timeout. Driver has to issue a separate command to wake
        PHY up.
        
    ==========================================================================
 */
VOID AsicSleepThenAutoWakeup(
    IN PRTMP_ADAPTER pAd, 
    IN USHORT TbttNumToNextWakeUp) 
{
    MAC_CSR11_STRUC csr11;
    
    // we have decided to SLEEP, so at least do it for a BEACON period.
    if (TbttNumToNextWakeUp == 0)
        TbttNumToNextWakeUp = 1;
    
    // set MAC_CSR11 for next wakeup
    csr11.word = 0;
    csr11.field.Sleep2AwakeLatency = 5;
    csr11.field.NumOfTBTTBeforeWakeup = TbttNumToNextWakeUp - 1;
    csr11.field.DelayAfterLastTBTTBeforeWakeup = pAd->PortCfg.BeaconPeriod - 10; // 5 TU ahead of desired TBTT
    
     //
    // To make sure ASIC Auto-Wakeup functionality works properly
    // We must disable it first to let ASIC recounting the Auto-Wakeup period.
    // After that enable it.
    //
    csr11.field.bAutoWakeupEnable = 0;  //Disable
    RTMP_IO_WRITE32(pAd, MAC_CSR11, csr11.word);
 
    csr11.field.bAutoWakeupEnable = 1;  //Enable
    RTMP_IO_WRITE32(pAd, MAC_CSR11, csr11.word);

    DBGPRINT(RT_DEBUG_TRACE, ">>>AsicSleepThenAutoWakeup(sleep %d TU)<<<\n", 
            (csr11.field.NumOfTBTTBeforeWakeup * pAd->PortCfg.BeaconPeriod) + csr11.field.DelayAfterLastTBTTBeforeWakeup);
    
    RTMP_IO_WRITE32(pAd, SOFT_RESET_CSR, 0x00000005);   // clear "Host-force-MAC-clock-ON" bit
    RTMP_IO_WRITE32(pAd, IO_CNTL_CSR, 0x0000001c);      // put "RF interface value" bit to power-save
    RTMP_IO_WRITE32(pAd, INT_MASK_CSR, 0x003f0a17);     // disable DMA interrupt
    RTMP_IO_WRITE32(pAd, PCI_USEC_CSR, 0x00000060);     // derive 1-us tick from PCI clock
    AsicSendCommandToMcu(pAd, 0x30, 0xff, 0x00, 0x00);   // send POWER-SAVE command to MCU

    OPSTATUS_SET_FLAG(pAd, fOP_STATUS_DOZE);
}

/*
    ==========================================================================
    Description:
        AsicForceWakeup() is used whenever manual wakeup is required
        AsicForceSleep() should only be used when not in INFRA BSS. When
        in INFRA BSS, we should use AsicSleepThenAutoWakeup() instead.
    ==========================================================================
 */
VOID AsicForceSleep(
    IN PRTMP_ADAPTER pAd)
{
    MAC_CSR11_STRUC csr11;

    DBGPRINT(RT_DEBUG_TRACE, ">>>AsicForceSleep<<<\n");

    // no auto wakeup
    csr11.word = 0;
    csr11.field.Sleep2AwakeLatency = 5;
    csr11.field.bAutoWakeupEnable = 0;
    RTMP_IO_WRITE32(pAd, MAC_CSR11, csr11.word);

    RTMP_IO_WRITE32(pAd, SOFT_RESET_CSR, 0x00000005);   // clear "Host-force-MAC-clock-ON" bit
    RTMP_IO_WRITE32(pAd, IO_CNTL_CSR, 0x0000001c);      // put "RF interface value" bit to power-save
    RTMP_IO_WRITE32(pAd, INT_MASK_CSR, 0x003f0a17);     // disable DMA interrupt
    RTMP_IO_WRITE32(pAd, PCI_USEC_CSR, 0x00000060);     // derive 1-us tick from PCI clock
    AsicSendCommandToMcu(pAd, 0x30, 0xff, 0x00, 0x00);

    OPSTATUS_SET_FLAG(pAd, fOP_STATUS_DOZE);
}

/*
    ==========================================================================
    Description:
        AsicForceWakeup() is used whenever Twakeup timer (set via AsicSleepThenAutoWakeup)
        expired.

    ==========================================================================
 */
VOID AsicForceWakeup(
    IN PRTMP_ADAPTER pAd)
{
    MAC_CSR11_STRUC csr11;

    DBGPRINT(RT_DEBUG_TRACE, ">>>AsicForceWakeup<<<\n");
    
    // cancel auto wakeup timer
    csr11.word = 0;
    csr11.field.Sleep2AwakeLatency = 5;
    csr11.field.bAutoWakeupEnable = 0;
    RTMP_IO_WRITE32(pAd, MAC_CSR11, csr11.word);

    RTMP_IO_WRITE32(pAd, SOFT_RESET_CSR, 0x00000007);       // turn on "Host force MAC clock ON" bit
    RTMP_IO_WRITE32(pAd, IO_CNTL_CSR, 0x00000018);          // put "RF interface value" to awake
    RTMP_IO_WRITE32(pAd, PCI_USEC_CSR, 0x00000020);         // derive 1-us tick from PCI clock
    AsicSendCommandToMcu(pAd, 0x31, 0xff, 0x00, 0x00);       // send WAKEUP command to MCU
    //
    // Can't enable interrupt here if ASIC wake up from sleep.
    // This will cause some interrupt occured and cause no one got service it.
    // then cause system hang. 
    // We will enable it on the regular routine NdisMSynchronizeWithInterrupt
    // before exit the ISR routine.
    //
    // RTMP_IO_WRITE32(pAd, INT_MASK_CSR, 0x0000ff00);         // re-enable DMA interrupt
    //

    OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_DOZE);

}

/*
    ==========================================================================
    Description:
        
    ==========================================================================
 */
VOID AsicSetBssid(
    IN PRTMP_ADAPTER pAd, 
    IN PUCHAR pBssid) 
{
    ULONG         Addr4;

    Addr4 = (ULONG)(pBssid[0])       | 
            (ULONG)(pBssid[1] << 8)  | 
            (ULONG)(pBssid[2] << 16) |
            (ULONG)(pBssid[3] << 24);
    RTMP_IO_WRITE32(pAd, MAC_CSR4, Addr4);

    // always one BSSID in STA mode
    Addr4 = (ULONG)(pBssid[4]) | (ULONG)(pBssid[5] << 8) | 0x00030000;
    RTMP_IO_WRITE32(pAd, MAC_CSR5, Addr4);
}

/*
    ==========================================================================
    Description:
   
    ==========================================================================
 */
VOID AsicDisableSync(
    IN PRTMP_ADAPTER pAd) 
{
    TXRX_CSR9_STRUC csr;
    DBGPRINT(RT_DEBUG_TRACE, "--->Disable TSF synchronization\n");

    // 2003-12-20 disable TSF and TBTT while NIC in power-saving have side effect
    //            that NIC will never wakes up because TSF stops and no more 
    //            TBTT interrupts
    RTMP_IO_READ32(pAd, TXRX_CSR9, &csr.word);
    csr.field.bBeaconGen = 0;
    csr.field.TsfSyncMode = 0;
    csr.field.bTBTTEnable = 0;
    csr.field.bTsfTicking = 0;
    RTMP_IO_WRITE32(pAd, TXRX_CSR9, csr.word);
}

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID AsicEnableBssSync(
    IN PRTMP_ADAPTER pAd) 
{
    TXRX_CSR9_STRUC csr;

    DBGPRINT(RT_DEBUG_TRACE, "--->AsicEnableBssSync(INFRA mode)\n");

    RTMP_IO_READ32(pAd, TXRX_CSR9, &csr.word);
//  RTMP_IO_WRITE32(pAd, TXRX_CSR9, 0x00000000);


    csr.field.BeaconInterval = pAd->PortCfg.BeaconPeriod << 4; // ASIC register in units of 1/16 TU
    csr.field.bTsfTicking = 1;
    csr.field.TsfSyncMode = 1; // sync TSF in INFRASTRUCTURE mode
    csr.field.bBeaconGen  = 0; // do NOT generate BEACON
    csr.field.bTBTTEnable = 1;
        
    RTMP_IO_WRITE32(pAd, TXRX_CSR9, csr.word);
}

/*
    ==========================================================================
    Description:
    Note: 
        BEACON frame in shared memory should be built ok before this routine
        can be called. Otherwise, a garbage frame maybe transmitted out every
        Beacon period.
 
    ==========================================================================
 */
VOID AsicEnableIbssSync(
    IN PRTMP_ADAPTER pAd)
{
    TXRX_CSR9_STRUC csr9;
    PUCHAR          ptr;
    UINT i;
    DBGPRINT(RT_DEBUG_TRACE, "--->AsicEnableIbssSync(ADHOC mode)\n");

    RTMP_IO_READ32(pAd, TXRX_CSR9, &csr9.word);
    csr9.field.bBeaconGen = 0;
    csr9.field.bTBTTEnable = 0;
    csr9.field.bTsfTicking = 0;
    RTMP_IO_WRITE32(pAd, TXRX_CSR9, csr9.word);

    RTMP_IO_WRITE32(pAd, HW_BEACON_BASE0, 0); // invalidate BEACON0 owner/valid bit to prevent garbage
    RTMP_IO_WRITE32(pAd, HW_BEACON_BASE1, 0); // invalidate BEACON1 owner/valid bit to prevent garbage
    RTMP_IO_WRITE32(pAd, HW_BEACON_BASE2, 0); // invalidate BEACON2 owner/valid bit to prevent garbage
    RTMP_IO_WRITE32(pAd, HW_BEACON_BASE3, 0); // invalidate BEACON3 owner/valid bit to prevent garbage

    // move BEACON TXD and frame content to on-chip memory
    ptr = (PUCHAR)&pAd->BeaconTxD;
    for (i=0; i<TXINFO_SIZE; i++)  // 24-byte TXINFO field
    {
        RTMP_IO_WRITE8(pAd, HW_BEACON_BASE0 + i, *ptr);
        ptr ++;
    }

    // start right after the 24-byte TXINFO field
    ptr = pAd->BeaconBuf;
    for (i=0; i< pAd->BeaconTxD.DataByteCnt; i++)
    {
        RTMP_IO_WRITE8(pAd, HW_BEACON_BASE0 + TXINFO_SIZE + i, *ptr); 
        ptr ++;
    }
    
    //
    // For Wi-Fi faily generated beacons between participating stations. 
    // Set TBTT phase adaptive adjustment step to 8us (default 16us)
    // 
	RTMP_IO_WRITE32(pAd, TXRX_CSR10, 0x00001008);
	
    // start sending BEACON
    csr9.field.BeaconInterval = pAd->PortCfg.BeaconPeriod << 4; // ASIC register in units of 1/16 TU
    csr9.field.bTsfTicking = 1;
    csr9.field.TsfSyncMode = 2; // sync TSF in IBSS mode
    csr9.field.bTBTTEnable = 1;
    csr9.field.bBeaconGen = 1;
    RTMP_IO_WRITE32(pAd, TXRX_CSR9, csr9.word);
}

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID AsicSetEdcaParm(
    IN PRTMP_ADAPTER pAd,
    IN PEDCA_PARM    pEdcaParm)
{
	AC_TXOP_CSR0_STRUC csr0;
    AC_TXOP_CSR1_STRUC csr1;
    AIFSN_CSR_STRUC    AifsnCsr;
    CWMIN_CSR_STRUC    CwminCsr;
    CWMAX_CSR_STRUC    CwmaxCsr;

    DBGPRINT(RT_DEBUG_TRACE,"AsicSetEdcaParm\n");
    if ((pEdcaParm == NULL) || (pEdcaParm->bValid == FALSE)|| (pAd->PortCfg.bWmmCapable == FALSE))
    {
        OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_WMM_INUSED);
        
        csr0.field.Ac0Txop = 0;     // QID_AC_BE
        csr0.field.Ac1Txop = 0;     // QID_AC_BK
        RTMP_IO_WRITE32(pAd, AC_TXOP_CSR0, csr0.word);
        if (pAd->PortCfg.PhyMode == PHY_11B)
        {
            csr1.field.Ac2Txop = cpu2le16(192);     // AC_VI: 192*32us ~= 6ms
            csr1.field.Ac3Txop = cpu2le16(96);      // AC_VO: 96*32us  ~= 3ms
        }
        else
        {
            csr1.field.Ac2Txop = cpu2le16(96);      // AC_VI: 96*32us ~= 3ms
            csr1.field.Ac3Txop = cpu2le16(48);      // AC_VO: 48*32us ~= 1.5ms
        }
        RTMP_IO_WRITE32(pAd, AC_TXOP_CSR1, csr1.word);

        CwminCsr.word = 0;
        CwminCsr.field.Cwmin0 = CW_MIN_IN_BITS;
        CwminCsr.field.Cwmin1 = CW_MIN_IN_BITS;
        CwminCsr.field.Cwmin2 = CW_MIN_IN_BITS;
        CwminCsr.field.Cwmin3 = CW_MIN_IN_BITS;
        RTMP_IO_WRITE32(pAd, CWMIN_CSR, CwminCsr.word);

        CwmaxCsr.word = 0;
        CwmaxCsr.field.Cwmax0 = CW_MAX_IN_BITS;
        CwmaxCsr.field.Cwmax1 = CW_MAX_IN_BITS;
        CwmaxCsr.field.Cwmax2 = CW_MAX_IN_BITS;
        CwmaxCsr.field.Cwmax3 = CW_MAX_IN_BITS;
        RTMP_IO_WRITE32(pAd, CWMAX_CSR, CwmaxCsr.word);

        RTMP_IO_WRITE32(pAd, AIFSN_CSR, 0x00002222);
        
        NdisZeroMemory(&pAd->PortCfg.APEdcaParm, sizeof(EDCA_PARM));
    }
    else
    {
        OPSTATUS_SET_FLAG(pAd, fOP_STATUS_WMM_INUSED);

		//
		// Modify Cwmin/Cwmax/Txop on queue[QID_AC_VI], Recommend by Jerry 2005/07/27
		// To degrade our VIDO Queue's throughput for WiFi WMM S3T07 Issue.
		//
		pEdcaParm->Txop[QID_AC_VI] = pEdcaParm->Txop[QID_AC_VI] * 7 / 10;
    
        csr0.field.Ac0Txop = cpu2le16(pEdcaParm->Txop[QID_AC_BE]);
        csr0.field.Ac1Txop = cpu2le16(pEdcaParm->Txop[QID_AC_BK]);
        RTMP_IO_WRITE32(pAd, AC_TXOP_CSR0, csr0.word);

        csr1.field.Ac2Txop = cpu2le16(pEdcaParm->Txop[QID_AC_VI]);
        csr1.field.Ac3Txop = cpu2le16(pEdcaParm->Txop[QID_AC_VO]);
        RTMP_IO_WRITE32(pAd, AC_TXOP_CSR1, csr1.word);

        CwminCsr.word = 0;
        CwminCsr.field.Cwmin0 = pEdcaParm->Cwmin[QID_AC_BE];
        CwminCsr.field.Cwmin1 = pEdcaParm->Cwmin[QID_AC_BK];
        CwminCsr.field.Cwmin2 = pEdcaParm->Cwmin[QID_AC_VI];
        CwminCsr.field.Cwmin3 = pEdcaParm->Cwmin[QID_AC_VO];
        RTMP_IO_WRITE32(pAd, CWMIN_CSR, CwminCsr.word);

        CwmaxCsr.word = 0;
	CwmaxCsr.field.Cwmax0 = pEdcaParm->Cwmax[QID_AC_BE];
        CwmaxCsr.field.Cwmax1 = pEdcaParm->Cwmax[QID_AC_BK];
        CwmaxCsr.field.Cwmax2 = pEdcaParm->Cwmax[QID_AC_VI];
        CwmaxCsr.field.Cwmax3 = pEdcaParm->Cwmax[QID_AC_VO];
        RTMP_IO_WRITE32(pAd, CWMAX_CSR, CwmaxCsr.word);

        AifsnCsr.word = 0;
        AifsnCsr.field.Aifsn0 = pEdcaParm->Aifsn[QID_AC_BE];
        AifsnCsr.field.Aifsn1 = pEdcaParm->Aifsn[QID_AC_BK];
        AifsnCsr.field.Aifsn2 = pEdcaParm->Aifsn[QID_AC_VI];
        AifsnCsr.field.Aifsn3 = pEdcaParm->Aifsn[QID_AC_VO];
        RTMP_IO_WRITE32(pAd, AIFSN_CSR, AifsnCsr.word);
        
        NdisMoveMemory(&pAd->PortCfg.APEdcaParm, pEdcaParm, sizeof(EDCA_PARM));
 
        DBGPRINT(RT_DEBUG_TRACE,"EDCA [#%d]: AIFSN CWmin CWmax  TXOP(us)  ACM\n", pEdcaParm->EdcaUpdateCount);
        DBGPRINT(RT_DEBUG_TRACE,"    AC_BE     %d     %d     %d     %4d     %d\n",
            pEdcaParm->Aifsn[0],
            pEdcaParm->Cwmin[0],
            pEdcaParm->Cwmax[0],
            pEdcaParm->Txop[0]<<5,
            pEdcaParm->bACM[0]);
        DBGPRINT(RT_DEBUG_TRACE,"    AC_BK     %d     %d     %d     %4d     %d\n",
            pEdcaParm->Aifsn[1],
            pEdcaParm->Cwmin[1],
            pEdcaParm->Cwmax[1],
            pEdcaParm->Txop[1]<<5,
            pEdcaParm->bACM[1]);
        DBGPRINT(RT_DEBUG_TRACE,"    AC_VI     %d     %d     %d     %4d     %d\n",
            pEdcaParm->Aifsn[2],
            pEdcaParm->Cwmin[2],
            pEdcaParm->Cwmax[2],
            pEdcaParm->Txop[2]<<5,
            pEdcaParm->bACM[2]);
        DBGPRINT(RT_DEBUG_TRACE,"    AC_VO     %d     %d     %d     %4d     %d\n",
            pEdcaParm->Aifsn[3],
            pEdcaParm->Cwmin[3],
            pEdcaParm->Cwmax[3],
            pEdcaParm->Txop[3]<<5,
            pEdcaParm->bACM[3]);
    }
}

/*
    ==========================================================================
    Description:
    ==========================================================================
 */
VOID AsicSetSlotTime(
    IN PRTMP_ADAPTER pAd,
    IN BOOLEAN bUseShortSlotTime) 
{
    MAC_CSR9_STRUC Csr9;

    //Big 225
    //bUseShortSlotTime = TRUE; // 2005-04-15 john

    if (bUseShortSlotTime && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED))
        return;
    else if ((!bUseShortSlotTime) && (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED)))
        return;
        
    if (bUseShortSlotTime)
        OPSTATUS_SET_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED);
    else
        OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED);
    
    RTMP_IO_READ32(pAd, MAC_CSR9, &Csr9.word);
#ifdef WIFI_TEST
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED))
    {
        if (pAd->PortCfg.PhyMode == PHY_11A)
            Csr9.field.SlotTime = 14;
        else
            Csr9.field.SlotTime = (bUseShortSlotTime)? 14 : 20;
    }
    else
    {
        if (pAd->PortCfg.PhyMode == PHY_11A)
            Csr9.field.SlotTime = 9;
        else
            Csr9.field.SlotTime = (bUseShortSlotTime)? 9 : 20;
    }
#else
	Csr9.field.SlotTime = (bUseShortSlotTime)? 9 : 20;
#endif

#ifndef WIFI_TEST
    // force using short SLOT time for FAE to demo performance when TxBurst is ON
    if (pAd->PortCfg.bEnableTxBurst)
        Csr9.field.SlotTime = 9;
#endif

     if (pAd->PortCfg.BssType == BSS_ADHOC)	
		Csr9.field.SlotTime = 20;

    RTMP_IO_WRITE32(pAd, MAC_CSR9, Csr9.word);

     // Bug 225
#if 0
	//
	// For some reasons, always set it to short slot time.
	// 
	// ToDo: Should consider capability with 11B
	//
	RTMP_IO_READ32(pAd, MAC_CSR9, &Csr9.word);
	Csr9.field.SlotTime = 9;
#endif

	RTMP_IO_WRITE32(pAd, MAC_CSR9, Csr9.word);

    DBGPRINT(RT_DEBUG_TRACE, "AsicSetSlotTime(=%d us)\n", Csr9.field.SlotTime);
}

/*
	==========================================================================
	Description:
		danamic tune BBP R17 to find a balance between sensibility and 
		noise isolation

	==========================================================================
 */
VOID AsicBbpTuning(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR R17, R17UpperBound, R17LowerBound;
	int dbm;

	// If turn on radar detection, BBP_R17 tuning needs to be turn off.
	if ((pAd->BbpTuning.bEnable == FALSE) || (pAd->PortCfg.RadarDetect.RDMode != RD_NORMAL_MODE))
		return;

	if(!INFRA_ON(pAd) && !ADHOC_ON(pAd))
		return;

	R17 = pAd->BbpWriteLatch[17];
	if (pAd->LatchRfRegs.Channel <= 14)
	{
		R17UpperBound = pAd->BbpTuning.R17UpperBoundG;
		R17LowerBound = pAd->BbpTuning.R17LowerBoundG;
	}
	else
	{
		R17UpperBound = pAd->BbpTuning.R17UpperBoundA;
		R17LowerBound = pAd->BbpTuning.R17LowerBoundA;
	}

	//
	// CASE 2. work as a STA
	//
	if (pAd->Mlme.CntlMachine.CurrState != CNTL_IDLE)  // no R17 tuning when SCANNING
		return;  

	dbm = pAd->PortCfg.AvgRssi - pAd->BbpRssiToDbmDelta;

	if ((pAd->RfIcType == RFIC_5325) || (pAd->RfIcType == RFIC_2529))
	{
		// choose greater rssi to do evaluation
		if (pAd->PortCfg.AvgRssi2 > pAd->PortCfg.AvgRssi)
		{
			dbm = pAd->PortCfg.AvgRssi2 - pAd->BbpRssiToDbmDelta;
		}
	}

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
#if 0
		// Rule 0. "long distance case"
		//   when RSSI is too weak, many signals will become false CCA thus affect R17 tuning.
		//   so in this case, just stop R17 tuning
		if ((dbm < -80) && (pAd->Mlme.PeriodicRound > 20))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("weak RSSI=%d, CCA=%d, stop tuning, R17 = 0x%x\n", 
				dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17));
			return;
		}

		// Rule 1. "special big-R17 for short-distance" when not SCANNING
		else 
#endif
		if (dbm >= RSSI_FOR_VERY_LOW_SENSIBILITY)
		{
			if (R17 != 0x60) // R17UpperBound)
			{
				R17 = 0x60; // R17UpperBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
			}
			DBGPRINT(RT_DEBUG_TRACE, "strong RSSI=%d, CCA=%d, fixed R17 at 0x%x\n", 
				dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
			return;
		}

		else if (dbm >= RSSI_FOR_LOW_SENSIBILITY)
		{
			if (R17 != R17UpperBound)
			{
				R17 = R17UpperBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
			}
			DBGPRINT(RT_DEBUG_TRACE, "strong RSSI=%d, CCA=%d, fixed R17 at 0x%x\n", 
				dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
			return;
		}

		else if (dbm >= RSSI_FOR_MID_LOW_SENSIBILITY)
		{
			if (R17 != (R17LowerBound + 0x10))
			{
				R17 = R17LowerBound + 0x10;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
			}
			DBGPRINT(RT_DEBUG_TRACE, "mid RSSI=%d, CCA=%d, fixed R17 at 0x%x\n", 
				dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
			return;
		}

		// Rule 2. "middle-R17 for mid-distance" when not SCANNING
		else if (dbm >= RSSI_FOR_MID_SENSIBILITY) 
		{
			if (R17 != (R17LowerBound + 0x08))
			{
				R17 = R17LowerBound + 0x08;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
			}
			DBGPRINT(RT_DEBUG_TRACE, "mid RSSI=%d, CCA=%d, fixed R17 at 0x%x\n", 
				dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
			return;
		}

		// Rule 2.1 originated from RT2500 Netopia case which changes R17UpperBound according
		//          to RSSI.
		else
		{
			// lower R17UpperBound when RSSI weaker than -70 dbm
			R17UpperBound -= 2*(RSSI_FOR_MID_SENSIBILITY - dbm);
			if (R17UpperBound < R17LowerBound)
				R17UpperBound = R17LowerBound;

			if (R17 > R17UpperBound)
			{
				R17 = R17UpperBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
				DBGPRINT(RT_DEBUG_TRACE, "RSSI=%d, CCA=%d, R17=R17UpperBound=0x%x\n", dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
				return;
			}

			// if R17 not exceeds R17UpperBound yet, then goes down to Rule 3 "R17 tuning based on False CCA"
		}

	}
	
	// Rule 3. otherwise, R17 is currenly in dynamic tuning range
	//    Keep dynamic tuning based on False CCA counter

	if ((pAd->RalinkCounters.OneSecFalseCCACnt > pAd->BbpTuning.FalseCcaUpperThreshold) &&
		(R17 < R17UpperBound))
	{
		R17 += pAd->BbpTuning.R17Delta;

		if (R17 >= R17UpperBound)
			R17 = R17UpperBound;
		
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
		DBGPRINT(RT_DEBUG_TRACE, "RSSI=%d, CCA=%d, ++R17= 0x%x\n", dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
	}
	else if ((pAd->RalinkCounters.OneSecFalseCCACnt < pAd->BbpTuning.FalseCcaLowerThreshold) &&
		(R17 > R17LowerBound))
	{
		R17 -= pAd->BbpTuning.R17Delta;

		if (R17 <= R17LowerBound)
			R17 = R17LowerBound;

		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, R17);
		DBGPRINT(RT_DEBUG_TRACE, "RSSI=%d, CCA=%d, --R17= 0x%x\n", dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
	}
	else if(dbm<-120 && pAd->RalinkCounters.OneSecFalseCCACnt>1000)
	{
		pAd->BbpTuning.R17LowerUpperSelect = 0;
		
		DBGPRINT(RT_DEBUG_TRACE, "==Strong Situation== RSSI=%d, CCA=%d, --R17= 0x%x L:%u U:%u threshold:%u delta:%u\n", dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17
			,R17LowerBound,R17UpperBound, pAd->BbpTuning.FalseCcaLowerThreshold,pAd->BbpTuning.R17Delta);
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, "RSSI=%d, CCA=%d, keep R17 at 0x%x\n", dbm, pAd->RalinkCounters.OneSecFalseCCACnt, R17);
	}

}

VOID AsicAddSharedKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         BssIndex,
    IN UCHAR         KeyIdx,
    IN UCHAR         CipherAlg,
    IN PUCHAR        pKey,
    IN PUCHAR        pTxMic,
    IN PUCHAR        pRxMic)
{
    INT   i;
    ULONG offset, csr0;
    SEC_CSR1_STRUC csr1;

    DBGPRINT(RT_DEBUG_TRACE, "AsicAddSharedKeyEntry: %s key #%d\n", CipherName[CipherAlg], BssIndex*4 + KeyIdx);
    DBGPRINT_RAW(RT_DEBUG_TRACE, "     Key =");
    for (i = 0; i < 16; i++)
    {
        printk("%02x:", pKey[i]);
    }
    printk("\n");
    if (pRxMic)
    {
        DBGPRINT_RAW(RT_DEBUG_TRACE, "     Rx MIC Key = ");
        for (i = 0; i < 8; i++)
        {
            printk("%02x:", pRxMic[i]);
        }
        printk("\n");
    }
    if (pTxMic)
    {
        DBGPRINT_RAW(RT_DEBUG_TRACE, "     Tx MIC Key = ");
        for (i = 0; i < 8; i++)
        {
            printk("%02x:", pTxMic[i]);
        }
        printk("\n");
    }
    
    //
    // fill key material - key + TX MIC + RX MIC
    //
    offset = SHARED_KEY_TABLE_BASE + (4*BssIndex + KeyIdx)*HW_KEY_ENTRY_SIZE;
    
    for (i=0; i<MAX_LEN_OF_SHARE_KEY; i++) 
    {
        RTMP_IO_WRITE8(pAd, offset + i, pKey[i]);
    }

    offset += MAX_LEN_OF_SHARE_KEY;
    if (pTxMic)
    {
        for (i=0; i<8; i++)
        {
            RTMP_IO_WRITE8(pAd, offset + i, pTxMic[i]);
        }
    }

    offset += 8;
    if (pRxMic)
    {
        for (i=0; i<8; i++)
        {
            RTMP_IO_WRITE8(pAd, offset + i, pRxMic[i]);
        }
    }

    //
    // Update cipher algorithm. STA always use BSS0
    //
    RTMP_IO_READ32(pAd, SEC_CSR1, &csr1.word);
    if (KeyIdx == 0)
        csr1.field.Bss0Key0CipherAlg = CipherAlg;
    else if (KeyIdx == 1)
        csr1.field.Bss0Key1CipherAlg = CipherAlg;
    else if (KeyIdx == 2)
	    csr1.field.Bss0Key2CipherAlg = CipherAlg;
    else
	    csr1.field.Bss0Key3CipherAlg = CipherAlg;
    RTMP_IO_WRITE32(pAd, SEC_CSR1, csr1.word);
    
    
    //
    // enable this key entry
    //
    RTMP_IO_READ32(pAd, SEC_CSR0, &csr0);
    csr0 |= BIT32[BssIndex*4 + KeyIdx];     // turrn on the valid bit
    RTMP_IO_WRITE32(pAd, SEC_CSR0, csr0);
    
}

VOID AsicRemoveSharedKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         BssIndex,
    IN UCHAR         KeyIdx)
{
    ULONG SecCsr0;

    DBGPRINT(RT_DEBUG_TRACE,"AsicRemoveSharedKeyEntry: #%d \n", BssIndex*4 + KeyIdx);

    ASSERT(BssIndex < 4);
    ASSERT(KeyIdx < SHARE_KEY_NUM);
    
    RTMP_IO_READ32(pAd, SEC_CSR0, &SecCsr0);
    SecCsr0 &= (~BIT32[BssIndex*4 + KeyIdx]); // clear the valid bit
    RTMP_IO_WRITE32(pAd, SEC_CSR0, SecCsr0);
}

VOID AsicAddPairwiseKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN PUCHAR        pAddr,
    IN UCHAR         KeyIdx,
    IN UCHAR         CipherAlg,
    IN PUCHAR        pKey,
    IN PUCHAR        pTxMic,
    IN PUCHAR        pRxMic)
{
    INT i;
    ULONG offset, csr2, csr3;

    DBGPRINT(RT_DEBUG_TRACE,"AsicAddPairwiseKeyEntry: #%d Alg=%s mac=%02x:%02x:%02x:%02x:%02x:%02x key=%02x-%02x-%02x-..\n",
        KeyIdx, CipherName[CipherAlg], pAddr[0], pAddr[1], pAddr[2],
        pAddr[3], pAddr[4], pAddr[5], pKey[0], pKey[1], pKey[2]);

    offset = PAIRWISE_KEY_TABLE_BASE + (KeyIdx * HW_KEY_ENTRY_SIZE);
    for (i=0; i<MAX_LEN_OF_PEER_KEY; i++)
    {
        RTMP_IO_WRITE8(pAd, offset + i, pKey[i]);
    }
    offset += MAX_LEN_OF_PEER_KEY;
    if (pTxMic)
    {
        for (i=0; i<8; i++)
        {
            RTMP_IO_WRITE8(pAd, offset+i, pTxMic[i]);
        }
    }
    offset += 8;
    if (pRxMic)
    {
        for (i=0; i<8; i++)
        {
            RTMP_IO_WRITE8(pAd, offset+i, pRxMic[i]);
        }
    }
    offset = PAIRWISE_TA_TABLE_BASE + (KeyIdx * HW_PAIRWISE_TA_ENTRY_SIZE);
    for (i=0; i<MAC_ADDR_LEN; i++)
    {
        RTMP_IO_WRITE8(pAd, offset+i, pAddr[i]);
    }
    RTMP_IO_WRITE8(pAd, offset+MAC_ADDR_LEN, CipherAlg);

    // enable this entry
    if (KeyIdx < 32)
    {
        RTMP_IO_READ32(pAd, SEC_CSR2, &csr2);
        csr2 |= BIT32[KeyIdx];
        RTMP_IO_WRITE32(pAd, SEC_CSR2, csr2);
    }
    else
    {
        RTMP_IO_READ32(pAd, SEC_CSR3, &csr3);
        csr3 |= BIT32[KeyIdx-32];
        RTMP_IO_WRITE32(pAd, SEC_CSR3, csr3);
    }
}

VOID AsicRemovePairwiseKeyEntry(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         KeyIdx)
{
    ULONG csr2, csr3;

    DBGPRINT(RT_DEBUG_INFO,"AsicRemovePairwiseKeyEntry: #%d \n", KeyIdx);
    
    // invalidate this entry
    if (KeyIdx < 32)
    {
        RTMP_IO_READ32(pAd, SEC_CSR2, &csr2);
        csr2 &= (~BIT32[KeyIdx]);
        RTMP_IO_WRITE32(pAd, SEC_CSR2, csr2);
    }
    else
    {
        RTMP_IO_READ32(pAd, SEC_CSR3, &csr3);
        csr3 &= (~BIT32[KeyIdx-32]);
        RTMP_IO_WRITE32(pAd, SEC_CSR3, csr3);
    }
}

BOOLEAN AsicSendCommandToMcu(
    IN PRTMP_ADAPTER pAd,
    IN UCHAR         Command,
    IN UCHAR         Token,
    IN UCHAR         Arg0,
    IN UCHAR         Arg1)
{
    HOST_CMD_CSR_STRUC H2MCmd;
    H2M_MAILBOX_STRUC  H2MMailbox;
    int	i;

    do
    {
    	RTMP_IO_READ32(pAd, H2M_MAILBOX_CSR, &H2MMailbox.word);
    	if (H2MMailbox.field.Owner == 0)
	    break;
    	RTMPusecDelay(2);
     	DBGPRINT(RT_DEBUG_TRACE, "AsicSendCommandToMcu::Mail box is busy\n");
    } while(i++ < 100);
 
	if (i >= 100)
	{
		DBGPRINT_ERR("H2M_MAILBOX still hold by MCU. command fail\n");
		return FALSE;
	}

	
    H2MMailbox.field.Owner    = 1;     // pass ownership to MCU
    H2MMailbox.field.CmdToken = Token;
    H2MMailbox.field.Arg1     = Arg1;
    H2MMailbox.field.Arg0     = Arg0;
    RTMP_IO_WRITE32(pAd, H2M_MAILBOX_CSR, H2MMailbox.word);
    
    H2MCmd.word               = 0;
    H2MCmd.field.InterruptMcu = 1;
    H2MCmd.field.HostCommand  = Command;
    RTMP_IO_WRITE32(pAd, HOST_CMD_CSR, H2MCmd.word);
    DBGPRINT(RT_DEBUG_TRACE, "SW interrupt MCU (cmd=0x%02x, token=0x%02x, arg1,arg0=0x%02x,0x%02x)\n", 
    	H2MCmd.field.HostCommand, Token, Arg1, Arg0);
    return TRUE;
}

/*
    ========================================================================

    Routine Description:
        Verify the support rate for different PHY type

    Arguments:
        pAd                 Pointer to our adapter

    Return Value:
        None
        
    ========================================================================
*/
VOID	RTMPCheckRates(
	IN		PRTMP_ADAPTER	pAd,
	IN OUT	UCHAR			SupRate[],
	IN OUT	UCHAR			*SupRateLen)
{
	UCHAR	RateIdx, i, j;
	UCHAR	NewRate[12], NewRateLen;
	
	NewRateLen = 0;
	
	if (pAd->PortCfg.PhyMode == PHY_11B)
		RateIdx = 4;
//  else if ((pAd->PortCfg.PhyMode == PHY_11BG_MIXED) && 
//      (pAd->PortCfg.BssType == BSS_ADHOC)           &&
//      (pAd->PortCfg.AdhocMode == 0))
//		RateIdx = 4;
	else
		RateIdx = 12;

	// Check for support rates exclude basic rate bit	
	for (i = 0; i < *SupRateLen; i++)
		for (j = 0; j < RateIdx; j++)
			if ((SupRate[i] & 0x7f) == RateIdTo500Kbps[j])
				NewRate[NewRateLen++] = SupRate[i];
			
	*SupRateLen = NewRateLen;
	NdisMoveMemory(SupRate, NewRate, NewRateLen);
}

/*
	========================================================================

	Routine Description:
		Set Rx antenna for software diversity

	Arguments:
		pAd					Pointer to our adapter
		Pair1				0: for E1;	1:for E2
		Pair2				0: for E4;	1:for E3

	Return Value:
		None

	========================================================================
*/
VOID AsicSetRxAnt(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			Pair1,
	IN UCHAR			Pair2)
{
	ULONG		data;
	UCHAR		R77;
	
	DBGPRINT(RT_DEBUG_INFO, "AsicSetRxAnt, pair1=%d, pair2=%d\n", Pair1, Pair2);
	
	if ((pAd->RfIcType == RFIC_2529) && (pAd->Antenna.field.NumOfAntenna == 0)
		&& (pAd->Antenna.field.RxDefaultAntenna != SOFTWARE_DIVERSITY) && (pAd->Antenna.field.TxDefaultAntenna != SOFTWARE_DIVERSITY))
	{
		if (Pair1 != 0xFF)
		{
			RTMP_IO_READ32(pAd, MAC_CSR13, &data);
			data &= ~0x00001010;	// clear Bit 4,12

			if (Pair1 == 0)			// pair1 Primary Ant  
			{//	Ant E1
				//data;
			}
			else
			{//	Ant E2
				data |= 0x10;
			}
			RTMP_IO_WRITE32(pAd, MAC_CSR13, data);
		}

		if (Pair2 != 0xFF)
		{
			RTMP_IO_READ32(pAd, MAC_CSR13, &data);
			data &= ~0x00000808;	// clear Bit 3,11

			if (Pair2 == 0)			// pair2 Primary Ant  
			{//	Ant E3
				data |= 0x08;
			}
			else
			{//	Ant E4
				//data;
			}
			RTMP_IO_WRITE32(pAd, MAC_CSR13, data);
		}
		DBGPRINT(RT_DEBUG_INFO, "AsicSetRxAnt, pair1=%d, pair2=%d, data=0x%x\n", Pair1, Pair2, data);
	}
	else if (pAd->RfIcType == RFIC_2527)
	{
		//UCHAR		R77;
	
		// Update antenna registers
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R77, &R77);

		R77	&= ~0x03;		// clear Bit 0,1
		if (Pair1 == 0)
		{
			R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
		}
		else
		{
			//R77;				// <Bit1:Bit0> = <0:0>
		}

		// Disable Rx
		RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);

		// enable RX of MAC block
	    RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x025eb032); // Staion not drop control frame will fail WiFi Certification.
	}
	else if ((pAd->RfIcType == RFIC_2529) && (pAd->Antenna.field.NumOfAntenna == 2))
	{
		//UCHAR		R77;
	
		// Update antenna registers
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R77, &R77);

		R77	&= ~0x03;		// clear Bit 0,1

		if (Pair1 == 0)
		{
			R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
		}
		else
		{
			//R77;				// <Bit1:Bit0> = <0:0>
		}

		// Disable Rx
		RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);

        RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x025eb032); // Staion not drop control frame will fail WiFi Certification.
	
	}
	else if ((pAd->RfIcType == RFIC_5225) || (pAd->RfIcType == RFIC_5325))
	{
		//UCHAR		R77;
	
		// Update antenna registers
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R77, &R77);

		R77	&= ~0x03;		// clear Bit 0,1
		
		//Support 11B/G/A			
		if (pAd->PortCfg.BandState == BG_BAND)
		{
			//Check Rx Anttena
			if (Pair1 == 0)
			{
				R77 = R77 | 0x03;	// <Bit1:Bit0> = <1:1>
			}
			else
			{
				//R77;				// <Bit1:Bit0> = <0:0>
			}
		}
		else //A_BAND
		{
			//Check Rx Anttena
			if (Pair1 == 0)
			{
				//R77;				// <Bit1:Bit0> = <0:0>
			}
			else
			{
				R77	= R77 | 0x03;	// <Bit1:Bit0> = <1:1>
			}
		}

		// Disable Rx
		RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R77, R77);

		// enable RX of MAC block
	    RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x025eb032);  // Staion not drop control frame will fail WiFi Certification.
	}
}

// switch to secondary RxAnt pair for a while to collect it's average RSSI
// also set a timeout routine to do the actual evaluation. If evaluation 
// result shows a much better RSSI using secondary RxAnt, then a official
// RX antenna switch is performed.
VOID AsicEvaluateSecondaryRxAnt(
    IN PRTMP_ADAPTER pAd) 
{
    DBGPRINT(RT_DEBUG_TRACE,"AntDiv - before evaluate Pair1-Ant (%d,%d), Pair2-Ant (%d,%d)\n",
    	pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair1SecondaryRxAnt,pAd->RxAnt.Pair2PrimaryRxAnt, pAd->RxAnt.Pair2SecondaryRxAnt);

	if (pAd->RfIcType == RFIC_2529)
	{
		AsicSetRxAnt(pAd, pAd->RxAnt.Pair1SecondaryRxAnt, pAd->RxAnt.Pair2SecondaryRxAnt);
	}
	else
	{
		AsicSetRxAnt(pAd, pAd->RxAnt.Pair1SecondaryRxAnt, 0xFF);
	}
		
    pAd->RxAnt.EvaluatePeriod = 1;
   	pAd->RxAnt.FirstPktArrivedWhenEvaluate = FALSE;
   	pAd->RxAnt.RcvPktNumWhenEvaluate = 0;

    // a one-shot timer to end the evalution
    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
		RTMPSetTimer(pAd, &pAd->RxAnt.RxAntDiversityTimer, 100);
    else  
		RTMPSetTimer(pAd, &pAd->RxAnt.RxAntDiversityTimer, 300);

}

// this timeout routine collect AvgRssi[SecondaryRxAnt] and decide the best Ant
VOID AsicRxAntEvalTimeout(
    IN	unsigned long data) 
{
    RTMP_ADAPTER	*pAd = (RTMP_ADAPTER *)data;
    UCHAR			temp;

    DBGPRINT(RT_DEBUG_TRACE,"After Eval,RSSI[0,1,2,3]=<%d,%d,%d,%d>,RcvPktNumWhenEvaluate=%d\n",
        (pAd->RxAnt.Pair1AvgRssi[0] >> 3), (pAd->RxAnt.Pair1AvgRssi[1] >> 3),
        (pAd->RxAnt.Pair2AvgRssi[0] >> 3), (pAd->RxAnt.Pair2AvgRssi[1] >> 3), pAd->RxAnt.RcvPktNumWhenEvaluate);

	// Do nothing if the driver is starting halt state.
	// This might happen when timer already been fired before cancel timer with mlmehalt
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		return;

	pAd->RxAnt.EvaluatePeriod = 2;

    // 1-db or more will we consider to switch antenna pair
    if (pAd->RfIcType == RFIC_2529)
    {
    	if ((pAd->RxAnt.RcvPktNumWhenEvaluate != 0) && (pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1SecondaryRxAnt] >= pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1PrimaryRxAnt]))
    	{// select PrimaryRxAntPair
    		pAd->RxAnt.Pair1LastAvgRssi = (pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1SecondaryRxAnt] >> 3);

    		temp = pAd->RxAnt.Pair1PrimaryRxAnt;
    		pAd->RxAnt.Pair1PrimaryRxAnt = pAd->RxAnt.Pair1SecondaryRxAnt;
    		pAd->RxAnt.Pair1SecondaryRxAnt = temp;
    	}
    	else
    	{
    		AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, 0xFF);
    	}
    	
	    if ((pAd->RxAnt.RcvPktNumWhenEvaluate != 0) && (pAd->RxAnt.Pair2AvgRssi[pAd->RxAnt.Pair2SecondaryRxAnt] >= pAd->RxAnt.Pair2AvgRssi[pAd->RxAnt.Pair2PrimaryRxAnt]))
	    {// select SecondaryRxAntPair
	    	pAd->RxAnt.Pair2LastAvgRssi = (pAd->RxAnt.Pair2AvgRssi[pAd->RxAnt.Pair2SecondaryRxAnt] >> 3);
	    	
    		temp = pAd->RxAnt.Pair2PrimaryRxAnt;
    		pAd->RxAnt.Pair2PrimaryRxAnt = pAd->RxAnt.Pair2SecondaryRxAnt;
    		pAd->RxAnt.Pair2SecondaryRxAnt = temp;
	    }
	    else
	    {
	    	AsicSetRxAnt(pAd, 0xFF, pAd->RxAnt.Pair2PrimaryRxAnt);
	    }

		// reset avg rssi to -95
	    pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1SecondaryRxAnt] = (-95) << 3;
	    pAd->RxAnt.Pair2AvgRssi[pAd->RxAnt.Pair2SecondaryRxAnt] = (-95) << 3;
    }
    else
    {
    	if ((pAd->RxAnt.RcvPktNumWhenEvaluate != 0) && (pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1SecondaryRxAnt] >= pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1PrimaryRxAnt]))
    	{// select PrimaryRxAntPair
    		temp = pAd->RxAnt.Pair1PrimaryRxAnt;
    		pAd->RxAnt.Pair1PrimaryRxAnt = pAd->RxAnt.Pair1SecondaryRxAnt;
    		pAd->RxAnt.Pair1SecondaryRxAnt = temp;

    		pAd->RxAnt.Pair1LastAvgRssi = (pAd->RxAnt.Pair1AvgRssi[pAd->RxAnt.Pair1SecondaryRxAnt] >> 3);
    	}
    	else
    	{
    		AsicSetRxAnt(pAd, pAd->RxAnt.Pair1PrimaryRxAnt, pAd->RxAnt.Pair2PrimaryRxAnt);
    	}
    }

    pAd->RxAnt.EvaluatePeriod = 0;

    DBGPRINT(RT_DEBUG_TRACE,"After Eval-Pair1 #%d,Pair2 #%d\n",pAd->RxAnt.Pair1PrimaryRxAnt,pAd->RxAnt.Pair2PrimaryRxAnt);
}

VOID MlmeRssiReportExec(
    IN	unsigned long data)
{
	RTMP_ADAPTER	*pAd = (RTMP_ADAPTER *)data;

	if ((pAd->RalinkCounters.LastOneSecRxOkDataCnt == 0) && (pAd->Mlme.bTxRateReportPeriod))
	{
		pAd->Mlme.bTxRateReportPeriod = FALSE;
		RTMP_IO_WRITE32(pAd, TXRX_CSR1, 0x9eb39eb3); 
	}

}
 
/*
    ========================================================================
    Routine Description:
        Station side, Auto TxRate faster train up timer call back function.
    Return Value:
        None
    ========================================================================
*/
VOID StaQuickResponeForRateUpExec(
    IN	unsigned long data) 
{
    PRTMP_ADAPTER   pAd = (PRTMP_ADAPTER)data;
    UCHAR	UpRate, DownRate, CurrRate;
	ULONG	TxTotalCnt, TxErrorRatio = 0;
    
	do
    {        
		//
		// Only link up will to do the TxRate faster trains up.
		//
		if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)) 
			break;

		TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount +
					 pAd->RalinkCounters.OneSecTxRetryOkCount +
					 pAd->RalinkCounters.OneSecTxFailCount;

        // skip this time that has no traffic in the past period
        if (TxTotalCnt == 0)
        {
            break;
        }
        
        // decide the next upgrade rate and downgrade rate, if any
		CurrRate = pAd->PortCfg.TxRate;
        if ((pAd->PortCfg.Channel > 14) ||      // must be in 802.11A band
        	(pAd->PortCfg.PhyMode == PHY_11G))  // G-only mode, no CCK rates available
        {
            if (Phy11ANextRateUpward[CurrRate] <= pAd->PortCfg.MaxTxRate)
				UpRate = Phy11ANextRateUpward[CurrRate];
        	else
        		UpRate = CurrRate;
            DownRate = Phy11ANextRateDownward[CurrRate];
        }
        else
        {
			if (pAd->PortCfg.MaxTxRate < RATE_FIRST_OFDM_RATE)
	        {
	        	if (Phy11BNextRateUpward[CurrRate] <= pAd->PortCfg.MaxTxRate)
					UpRate = Phy11BNextRateUpward[CurrRate];
	        	else
	        		UpRate = CurrRate;
	            DownRate = Phy11BNextRateDownward[CurrRate];
	        }
	        else 
	        {
	        	if (Phy11BGNextRateUpward[CurrRate] <= pAd->PortCfg.MaxTxRate)
					UpRate = Phy11BGNextRateUpward[CurrRate];
	        	else
	        		UpRate = CurrRate;
	            DownRate = Phy11BGNextRateDownward[CurrRate];
	        }
        }

        //
        // PART 1. Decide TX Quality
        //   decide TX quality based on Tx PER when enough samples are available
        //
        if (TxTotalCnt > 15)
        {
			TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
            // downgrade TX quality if PER >= Rate-Down threshold
            if (TxErrorRatio >= RateDownPER[CurrRate])
            {
                pAd->DrsCounters.TxQuality[CurrRate] = DRS_TX_QUALITY_WORST_BOUND;
            }
        }

        pAd->DrsCounters.PER[CurrRate] = (UCHAR)TxErrorRatio;

        //
        // PART 2. Perform TX rate switching
        //   perform rate switching
        //
		if ((pAd->DrsCounters.TxQuality[CurrRate] >= DRS_TX_QUALITY_WORST_BOUND) && (CurrRate != DownRate))
        {
            pAd->PortCfg.TxRate = DownRate;
        }
        // PART 3. Post-processing if TX rate switching did happen
        //     if rate-up happen, clear all bad history of all TX rates
        //     if rate-down happen, only clear DownRate's bad history
        
        if (pAd->PortCfg.TxRate < CurrRate)
        {
            DBGPRINT(RT_DEBUG_TRACE,"StaQuickResponeForRateUpExec: Before TX rate = %d Mbps, Now Tx rate = %d Mbps\n", RateIdToMbps[CurrRate], RateIdToMbps[pAd->PortCfg.TxRate]);

            // shorter stable time require more penalty in next rate UP criteria
            //if (pAd->DrsCounters.CurrTxRateStableTime < 4)      // less then 4 sec
            //    pAd->DrsCounters.TxRateUpPenalty = DRS_PENALTY; // add 8 sec penalty
            //else if (pAd->DrsCounters.CurrTxRateStableTime < 8) // less then 8 sec
            //    pAd->DrsCounters.TxRateUpPenalty = 2;           // add 2 sec penalty
            //else
                pAd->DrsCounters.TxRateUpPenalty = 0;           // no penalty

            pAd->DrsCounters.CurrTxRateStableTime = 0;
            pAd->DrsCounters.LastSecTxRateChangeAction = 2; // rate down
            pAd->DrsCounters.TxQuality[pAd->PortCfg.TxRate] = 0;
            pAd->DrsCounters.PER[pAd->PortCfg.TxRate] = 0;
        }
        else
            pAd->DrsCounters.LastSecTxRateChangeAction = 0; // rate no change
        
        // reset all OneSecxxx counters
        pAd->RalinkCounters.OneSecTxFailCount = 0;
        pAd->RalinkCounters.OneSecTxNoRetryOkCount = 0;
        pAd->RalinkCounters.OneSecTxRetryOkCount = 0;
	} while (FALSE);
	pAd->PortCfg.QuickResponeForRateUpTimerRunning = FALSE;
}

/*
	========================================================================

	Routine Description:
		Radar detection routine

	Arguments:
		pAd     Pointer to our adapter

	Return Value:

	========================================================================
*/
VOID RadarDetectionStart(
	IN PRTMP_ADAPTER	pAd)
{
	DBGPRINT(RT_DEBUG_TRACE, "RadarDetectionStart--->\n");
		
	RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0257b032);        // Disable Rx
		
	// Set all relative BBP register to enter into radar detection mode
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R82, 0x20);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R83, 0);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R84, 0x40);

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R18, &pAd->PortCfg.RadarDetect.BBPR18);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R18, 0xFF);
	
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R21, &pAd->PortCfg.RadarDetect.BBPR21);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R21, 0x3F);
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R22, &pAd->PortCfg.RadarDetect.BBPR22);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R22, 0x3F);
	
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R16, &pAd->PortCfg.RadarDetect.BBPR16);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R16, 0xBD);
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R17, &pAd->PortCfg.RadarDetect.BBPR17);
	if (pAd->NicConfig2.field.ExternalLNAForA)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, 0x44); // if external LNA enable, this value need to be offset 0x10
	}
	else
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, 17, 0x34);
	}

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R64, &pAd->PortCfg.RadarDetect.BBPR64);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, 0x21);

	RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x0256b032);       // enable RX of MAC block
}

/*
	========================================================================

	Routine Description:
		Radar detection routine

	Arguments:
		pAd     Pointer to our adapter

	Return Value:
		TRUE    Found radar signal
		FALSE   Not found radar signal

	========================================================================
*/
BOOLEAN RadarDetectionStop(
	IN PRTMP_ADAPTER	pAd)
{
	UCHAR	R66;

	// Need to read the result of detection first
	// If restore all registers first, then the result will be reset.
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R66, &R66);

	// Restore all relative BBP register to exit radar detection mode
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R16, pAd->PortCfg.RadarDetect.BBPR16);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R17, pAd->PortCfg.RadarDetect.BBPR17);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R18, pAd->PortCfg.RadarDetect.BBPR18);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R21, pAd->PortCfg.RadarDetect.BBPR21);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R22, pAd->PortCfg.RadarDetect.BBPR22);
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, pAd->PortCfg.RadarDetect.BBPR64);

	if (R66 == 1)
		return TRUE;
	else
		return FALSE;
}

/*
	========================================================================

	Routine Description:
		Radar channel check routine

	Arguments:
		pAd     Pointer to our adapter

	Return Value:
		TRUE    need to do radar detect
		FALSE   need not to do radar detect

	========================================================================
*/
BOOLEAN RadarChannelCheck(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			Ch)
{
	INT		i;
	UCHAR	Channel[15]={52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140};

	for (i=0; i<15; i++)
	{
		if (Ch == Channel[i])
		{
			break;
		}
	}
	
	if (i != 15)
		return TRUE;
	else
		return FALSE;
}

/*
    ========================================================================
    Routine Description:
        Set/reset MAC registers according to bPiggyBack parameter
        
    Arguments:
    	pAd			- Adapter pointer
    	bPiggyBack	- Enable / Disable Piggy-Back
        
    Return Value:
        None
        
    ========================================================================
*/
VOID RTMPSetPiggyBack(
	IN PRTMP_ADAPTER	pAd,
	IN PMAC_TABLE_ENTRY pEntry,
	IN BOOLEAN			bPiggyBack)
{
	ULONG			csr0;
	QOS_CSR0_STRUC	qos_csr0;
	QOS_CSR1_STRUC	qos_csr1;

	RTMP_IO_READ32(pAd, PHY_CSR0, &csr0);
	csr0 = csr0 & 0xfff3ffff;				// Mask off bit 18, bit 19

	qos_csr0.word = 0;
	qos_csr1.word = 0;
	
	if (pEntry && bPiggyBack)
	{
		csr0 |= (BIT32[18] | BIT32[19]);		// b18, b19 to enable piggy-back

		qos_csr0.field.Byte0 = pEntry->Addr[0];
		qos_csr0.field.Byte1 = pEntry->Addr[1];
		qos_csr0.field.Byte2 = pEntry->Addr[2];
		qos_csr0.field.Byte3 = pEntry->Addr[3];

		qos_csr1.field.Byte4 = pEntry->Addr[4];
		qos_csr1.field.Byte5 = pEntry->Addr[5];
	}
	
	RTMP_IO_WRITE32(pAd, PHY_CSR0, csr0);
	RTMP_IO_WRITE32(pAd, QOS_CSR0, qos_csr0.word);
	RTMP_IO_WRITE32(pAd, QOS_CSR1, qos_csr1.word);
}

