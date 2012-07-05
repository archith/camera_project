#ifndef __RTL8139_H__
#define __RTL8139_H__

enum RTL8139_registers {
	MAC0		= 0x00,	/* Ethernet hardware address. */
	MAR0		= 0x08,	/* Multicast filter. */
	TxStatus0	= 0x10,	/* Transmit status (Four 32bit registers). */
	TxAddr0		= 0x20,	/* Tx descriptors (also four 32bit). */
	RxBuf		= 0x30,
	ChipCmd		= 0x37,
	RxBufPtr	= 0x38,
	RxBufAddr	= 0x3A,
	IntrMask	= 0x3C,
	IntrStatus	= 0x3E,
	TxConfig	= 0x40,
	ChipVersion	= 0x43,
	RxConfig	= 0x44,
	Timer		= 0x48,	/* A general-purpose counter. */
	RxMissed	= 0x4C,	/* 24 bits valid, write clears. */
	Cfg9346		= 0x50,
	Config0		= 0x51,
	Config1		= 0x52,
	FlashReg	= 0x54,
	MediaStatus	= 0x58,
	Config3		= 0x59,
	Config4		= 0x5A,	/* absent on RTL-8139A */
	HltClk		= 0x5B,
	MultiIntr	= 0x5C,
	TxSummary	= 0x60,
	BasicModeCtrl	= 0x62,
	BasicModeStatus	= 0x64,
	NWayAdvert	= 0x66,
	NWayLPAR	= 0x68,
	NWayExpansion	= 0x6A,
	FIFOTMS		= 0x70,	/* FIFO Control and test. */
	CSCR		= 0x74,	/* Chip Status and Configuration Register. */
	PARA78		= 0x78,
	PARA7c		= 0x7c,	/* Magic transceiver parameter register. */
	Config5		= 0xD8,	/* absent on RTL-8139A */
};

/*
 * Receive Status Register in RX packet header
 */
enum RxStatusBits {
	RxMulticast	= (1 << 15),
	RxPhysical	= (1 << 14),
	RxBroadcast	= (1 << 13),
	RxBadSymbol	= (1 << 5),
	RxRunt		= (1 << 4),
	RxTooLong	= (1 << 3),
	RxCRCErr	= (1 << 2),
	RxBadAlign	= (1 << 1),
	RxStatusOK	= (1 << 0),
};

/*
 * Transmit Status Register bits - TxStatus0 (0x10)
 */
enum TxStatusBits {
	TxCarrierLost	= (1 << 31),
	TxAborted	= (1 << 30),
	TxOutOfWindow	= (1 << 29),
	TxEarlyThreshold= (1 << 16),
	TxStatOK	= (1 << 15),
	TxUnderrun	= (1 << 14),
	TxHostOwns	= (1 << 13),
};

#define TX_FIFO_THRESH 256
#define TxFlag  ((TX_FIFO_THRESH << 11) & 0x003f0000)

/*
 * Command Register bits - ChipCmd (0x37)
 */
enum ChipCmdBits {
	CmdReset	= (1 << 4),
	CmdRxEnb	= (1 << 3),
	CmdTxEnb	= (1 << 2),
	RxBufEmpty	= (1 << 0),
};

/*
 * Interrupt Register bits - IntrStatus (0x3E)
 */

enum IntrStatusBits {
	PCIErr		= (1 << 15),
	PCSTimeout	= (1 << 14),
	RxFIFOOver	= (1 << 6),
	RxUnderrun	= (1 << 5),
	RxOverflow	= (1 << 4),
	TxErr		= (1 << 3),
	TxOK		= (1 << 2),
	RxErr		= (1 << 1),
	RxOK		= (1 << 0),

	TxAckBits	= (TxErr | TxOK),
	TxErrBits	= (TxErr),
	RxAckBits	= (RxFIFOOver | RxUnderrun | RxOverflow | RxErr | RxOK),
	RxErrBits	= (RxFIFOOver | RxUnderrun | RxOverflow | RxErr),
};

#define RTL8139IntrMask (PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver | TxErr | TxOK | RxErr | RxOK)

/*
 * Transmit Configuration Register bits - TxConfig (0x40)
 */

enum TxConfigBits {
	TxIFG1		= (1 << 25),
	TxIFG0		= (1 << 24),
	TxLoopBack	= (1 << 18) | (1 << 17),
	TxCRC		= (1 << 16),
	TxClearAbt	= (1 << 0),

	TxDMAShift	= 8,
	TxRetryShift	= 4,
};

#define TX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define TX_RETRY	8	/* 0-15.  retries = 16 + (TX_RETRY * 16) */
#define RTL8139TxConfig	((TX_DMA_BURST << TxDMAShift) | (TX_RETRY << TxRetryShift))

/*
 * Receive Configuration Register bits - RxConfig (0x44)
 */

enum RxConfigBits {
	RxNoWrap	= (1 << 7),
	AcceptErr	= (1 << 5),
	AcceptRunt	= (1 << 4),
	AcceptBroadcast	= (1 << 3),
	AcceptMulticast	= (1 << 2),
	AcceptMyPhys	= (1 << 1),
	AcceptAllPhys	= (1 << 0),

	RxFIFOThlShift	= 13,
	RxBufLenShift	= 11,
	RxDMAShift	= 8,
};

#define RTL8139RxConfig	((7 << RxFIFOThlShift) | (2 << RxBufLenShift) | (7 << RxDMAShift) | RxNoWrap | AcceptBroadcast | AcceptMyPhys)

#include <net.h>
int rtl8139_send (struct sk_buff *skb);
int rtl8139_recv (void);
int rtl8139_open (void);
int rtl8139_init (void);
int rtl8139_reset (void);

#endif // __RTL8139_H__
