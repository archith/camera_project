/**********************************************************************
 *
 * Filename:    UART_Regs.h
 *
 **********************************************************************/
/*----------------*
  Registers masks
 *----------------*/
// Baud Rate Clock Register mask
#define BRCR_E		0x80000000
#define BRCR_BSEL 0x0F000000
#define BRCR_BDIV 0x001F0000
#define BRCR_BCMP 0x0000FFFF

// Line Coding Register mask
#define LCR_MS		0xF0000000
#define LCR_SI    0x00020000
#define LCR_SO    0x00010000
#define LCR_X			0x00008000
#define LCR_L			0x00004000
#define LCR_ISDV	0x00003F00
#define LCR_DSZ		0x00000060
#define LCR_PAR		0x0000001C
#define LCR_STP		0x00000003

// Xon/Xoff Symbol Register mask
#define XXSR_XOFF 0x00FF0000
#define XXSR_XON	0x000000FF

// Interrupt Control/Status Register mask
#define ICSR_I		0x80000000
#define ICSR_Ti		0x40000000
#define ICSR_Ri		0x20000000
#define ICSR_Z1		0x04000000
#define ICSR_Z0		0x01000000
#define ICSR_E		0x00008000
#define ICSR_T		0x00004000
#define ICSR_R		0x00002000
#define ICSR_C1		0x00000C00
#define ICSR_C0		0x00000300
#define ICSR_RIDL 0x000000FF

// Transmitter Command/Status Register mask
#define TCSR_CMD	0xF0000000
#define TCSR_F		0x00008000
#define TCSR_E		0x00004000
#define TCSR_K		0x00002000
#define TCSR_B		0x00001000

// Transmitter Data/Count Register mask
#define TDCR_ST			0xC0000000
#define TDCR_TXDCNT 0x0FFF0000
#define TDCR_ERR		0x0000C000
#define TDCR_DATA		0x000000FF

// Receiver Command/Status Register mask
#define RCSR_CMD	0xF0000000
#define RCSR_Oe		0x00008000
#define RCSR_Fe		0x00004000
#define RCSR_Pe		0x00002000
#define RCSR_B		0x00001000
#define RCSR_Sb		0x00000800
#define RCSR_V		0x00000100
#define RCSR_DATA 0x000000FF

// Receiver DMA Control/Status Register mask
#define RDCSR_ST	0xC0000000
#define RDCSR_H		0x10000000
#define RDCSR_HWM 0x0FFF0000
#define RDCSR_Oe	0x00008000
#define RDCSR_Fe	0x00004000
#define RDCSR_Pe	0x00002000
#define RDCSR_RC	0x00000FFF

// ACIO0 Control/Status Register mask
#define A0CSR_V 0x00010000
#define A0CSR_M 0x00000018
#define A0CSR_S 0x00000007

// ACIO1 Control/Status Register mask
#define A1CSR_V 0x00010000
#define A1CSR_M 0x00000018
#define A1CSR_S 0x00000007

// Misc
#define TRUE	1
#define FALSE 0

typedef char						BOOL;
typedef unsigned char		UCHAR;
typedef unsigned short	USHORT;
typedef unsigned int		UINT;

/*-------------------*
  Register structure
 *-------------------*/
typedef struct					_UART_REGS
{
	UINT	BaudRateClock;						// 0x00
	UINT	LineCoding;								// 0x04
	UINT	XonXoffSymbol;						// 0x08
	UINT	InterruptControlStatus;		// 0x0C
	UINT	TransmitterCommandStatus; // 0x10
	UINT	TransmitterDataCount;			// 0x14
	UINT	ReceiverCommandStatus;		// 0x18
	UINT	ReceiverDMAControlStatus; // 0x1C
	UINT	ACIO0ControlStatus;				// 0x20
	UINT	ACIO1ControlStatus;				// 0x24
} UART_REGS;

#if 0
#define dbg(format, arg...) \
do { \
	printk( format, ## arg); \
} while (0)
#else
#define dbg(format, arg...)
#endif

/*--------------------*
  Functions Prototype
 *--------------------*/
void	SetBaudRateClockReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetBaudRateClockReg(volatile UART_REGS	*pRegs);
void	SetLineCodingReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetLineCodingReg(volatile UART_REGS	*pRegs);
void	SetXonXoffSymbolReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetXonXoffSymbolReg(volatile UART_REGS	*pRegs);
void	SetInterruptControlStatusReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetInterruptControlStatusReg(volatile UART_REGS	*pRegs);
void	SetTransmitterCommandStatusReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetTransmitterCommandStatusReg(volatile UART_REGS	*pRegs);
void	SetTransmitterDataCountReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetTransmitterDataCountReg(volatile UART_REGS	*pRegs);
void	SetReceiverCommandStatusReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetReceiverCommandStatusReg(volatile UART_REGS	*pRegs);
void	SetReceiverDMAControlStatusReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetReceiverDMAControlStatusReg(volatile UART_REGS	*pRegs);
void	SetACIO0ControlStatusReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetACIO0ControlStatusReg(volatile UART_REGS	*pRegs);
void	SetACIO1ControlStatusReg(volatile UART_REGS	*pRegs, UINT val);
UINT	GetACIO1ControlStatusReg(volatile UART_REGS	*pRegs);
