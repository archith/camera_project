/****************************************************************************
** COPYRIGHT (C) 2002 ZYDAS CORPORATION                                    **
** HTTP://WWW.ZYDAS.COM.TW/                                                **
****************************************************************************/

#ifndef _ZDEQUATES_H
#define _ZDEQUATES_H
//#include "zd1205.h"     //ivan

//-------------------------------------------------------------------------
// Ethernet Frame_t Sizes
//-------------------------------------------------------------------------
#define ETHERNET_ADDRESS_LENGTH         6
#define ETHERNET_HEADER_SIZE            14
#define MINIMUM_ETHERNET_PACKET_SIZE    60
#define MAXIMUM_ETHERNET_PACKET_SIZE    1514
#define	MAX_WLAN_SIZE					2432
#define PLCP_HEADER						4
#define WLAN_HEADER						24	
#define CRC32_LEN						4
#define IV_SIZE							4
#define ICV_SIZE						4	
#define EXTRA_INFO_LEN					4 
#define EXTEND_IV_LEN					4
#define MIC_LENGTH						8
#define WDS_ADD_HEADER					6
#define EAPOL_TYPE						0x888e

#define BCN_INTERVAL_OFFSET				8
#define CAP_OFFSET						10
#define SSID_OFFSET						12
#define	NUM_SUPPORTED_RATE				32

#define BSS_INFO_NUM					16

#define TUPLE_CACHE_SIZE				8//16
#define MAX_DEFRAG_NUM					8

#define MAX_RX_TIMEOUT					(512*10*1000)

#define	GCT_MAX_TX_PWR_SET				0x3f
#define	GCT_MIN_TX_PWR_SET				0x0

#define	AL2210_MAX_TX_PWR_SET			0xff
#define	AL2210_MIN_TX_PWR_SET			0x60

#define	TRACKING_NUM					10

// For Rate Adaption
#define FALL_RATE						0x0
#define	RISE_RATE						0x1

#define PS_CAM							0x0
#define	PS_PSM							0x1

#define	ACC_1							0x0
#define	ACC_2							0x1

// MAC_PA_STATE
#define	MAC_INI							0x0
#define	MAC_OPERATION					0x1

// RF TYPE
#define	AL2210MPVB_RF				0x4
#define	THETA_RF					0x6
#define	AL2210_RF					0x7
#define	MAXIM_NEW_RF				0x8
#define	GCT_RF						0x9
#define	PV2000_RF					0xA
#define	RALINK_RF					0xB
#define	INTERSIL_RF					0xC
#define	RFMD_RF						0xD
#define	MAXIM_RF					0xE
#define	PHILIPS_RF					0xF

#define ELEID_SSID					0
#define	ELEID_SUPRATES				1
#define ELEID_DSPARMS				3
#define ELEID_TIM					5
#define ELEID_ERP_INFO				42
#define ELEID_EXT_RATES				50
//-------------------------------------------------------------------------
//- Miscellaneous Equates
//-------------------------------------------------------------------------
#ifndef FALSE
#define FALSE       0
#define TRUE        1
#endif

#define DRIVER_NULL ((uint32)0xffffffff)

//-------------------------------------------------------------------------
// Bit Mask definitions
//-------------------------------------------------------------------------
#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000
#define BIT_16      0x00010000
#define BIT_17      0x00020000
#define BIT_18      0x00040000
#define BIT_19      0x00080000
#define BIT_20      0x00100000
#define BIT_21		0x00200000
#define BIT_22		0x00400000
#define BIT_23		0x00800000
#define BIT_24      0x01000000
#define BIT_25		0x02000000
#define BIT_26		0x04000000
#define BIT_27		0x08000000
#define BIT_28      0x10000000
#define BIT_31      0x80000000

#define BIT_0_1		0x0003
#define BIT_0_2     0x0007
#define BIT_0_3     0x000F
#define BIT_0_4     0x001F
#define BIT_0_5     0x003F
#define BIT_0_6     0x007F
#define BIT_0_7     0x00FF
#define BIT_0_8     0x01FF
#define BIT_0_13    0x3FFF
#define BIT_0_15    0xFFFF
#define BIT_1_2     0x0006
#define BIT_1_3     0x000E
#define BIT_2_5     0x003C
#define BIT_3_4     0x0018
#define BIT_4_5     0x0030
#define BIT_4_6     0x0070
#define BIT_4_7     0x00F0
#define BIT_5_7     0x00E0
#define BIT_5_9     0x03E0
#define BIT_5_12    0x1FE0
#define BIT_5_15    0xFFE0
#define BIT_6_7     0x00c0
#define BIT_7_11    0x0F80
#define BIT_8_10    0x0700
#define BIT_9_13    0x3E00
#define BIT_12_15   0xF000

#define BIT_16_20   0x001F0000
#define BIT_21_25   0x03E00000
#define BIT_26_27   0x0C000000


#define NO_WEP		0x0
#define AES			0x1
#define WEP64		0x2
#define WEP128		0x3
#define	TKIP		0x4

#define RANDOM		0x0
#define INCREMENT	0x1


// Device Bus-Master (Tx) state
#define TX_IDLE					0x00
#define TX_READ_TBD				0x01
#define TX_READ_DATA0			0x02
#define TX_READ_DATA1			0x03
#define TX_CHK_TBD_CNT			0x04
#define TX_READ_TCB				0x05
#define TX_CHK_TCB				0x06
#define TX_WAIT_DATA			0x07
#define TX_RETRYFAILURE			0x08
#define TX_REDOWNLOAD			0x09

// Device Bus-Master (Rx) state
#define RX_IDLE					0x00
#define RX_WAIT_DATA			0x10
#define RX_DATA0				0x20
#define RX_DATA1				0x30
#define RX_READ_RCB				0x50
#define RX_CHK_RCB				0x60
#define RX_WAIT_STS				0x70
#define RX_FRM_ERR				0x80
#define RX_CHK_DATA				0x90


#define MAX_SSID_LEN			32

#define HOST_PEND	BIT_31
#define CAM_WRITE	BIT_31
#define MAC_LENGTH	6

enum Encry_Type {
	ENCRY_NO = 0,
	ENCRY_WEP64,
	ENCRY_TKIP,
	ENCRY_RESERVED,
	ENCRY_CCMP,
	ENCRY_WEP128,
	ENCRY_WEP256
};

enum Operation_Mode {
	CAM_IBSS = 0,
	CAM_AP,
	CAM_STA,
	CAM_AP_WDS,
	CAM_AP_CLIENT,
	CAM_AP_VAP
};

#if 0
enum Frame_Control_Bit {
	TO_DS = BIT_0,
	FROM_DS = BIT_1,
	MORE_FRAG = BIT_2,
	RETRY_BIT = BIT_3,
	PWR_BIT = BIT_4,
	MORE_DATA = BIT_5,
	WEP_BIT = BIT_6,
	ORDER_BIT = BIT_7
};
#endif

#define MAX_USER				40
#define MAX_KEY_LENGTH			16
#define ENCRY_TYPE_START_ADDR	60
#define DEFAULT_ENCRY_TYPE		65	
#define KEY_START_ADDR			66
#define STA_KEY_START_ADDR		386
#define COUNTER_START_ADDR		418
#define STA_COUNTER_START_ADDR	423

#define EXTENDED_IV				BIT_5
#define QoS_DATA				BIT_7
#define TO_DS_FROM_DS			BIT_0_1

#define AP_MODE					BIT_24
#define IBSS_MODE				BIT_25
#define POWER_MNT				BIT_26
#define STA_PS					BIT_27

#define NON_ERP_PRESENT_BIT		BIT_0

#define	BEACON_TIME				1
#endif      // _EQUATES_H

