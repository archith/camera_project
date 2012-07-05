/**********************************************************************
 *
 * Filename:    UART_Utils.h
 *
 **********************************************************************/
typedef struct	_BaudRateClock
{
	UINT	rate;
	UINT	BSEL;
	UINT	BDIV;
	UINT	BCMP;
} BaudRateClock;

typedef struct	_LineCodingStru
{
	UINT	MS;
  UINT  Si;
  UINT  So;
	UINT	X;
	UINT	L;
	UINT	ISDV;
	UINT	DSZ;
	UINT	PAR;
	UINT	STP;
} LineCodingStru;

typedef struct	_TransmitterDMAStatus
{
	UINT	ST;
	UINT	TXDCNT;
	UINT	ERR;
} TransmitterDMAStatus;

typedef struct	_ReceiverStatusData
{
	USHORT	DATA;
	USHORT	V;
	USHORT	Sb;
	USHORT	B;
	USHORT	Pe;
	USHORT	Fe;
	USHORT	Oe;
} ReceiverStatusData;

typedef struct	_ReceiverDMAStatus
{
	UINT	RC;
	UCHAR Pe;
	UCHAR Fe;
	UCHAR Oe;
	UCHAR H;
	UCHAR ST;
} ReceiverDMAStatus;

typedef struct	_ACIOStatus
{
	UCHAR S;
	UCHAR V;
} ACIOStatus;

typedef struct	_InterruptControl
{
	UCHAR RIDL;
	UCHAR C0;
	UCHAR C1;
	UCHAR R;
	UCHAR T;
	UCHAR E;
} InterruptControl;

typedef struct	_InterruptStatus
{
	UCHAR Z0;
	UCHAR Z1;
	UCHAR Ri;
	UCHAR Ti;
	UCHAR I;
} InterruptStatus;

/*--------------------*
  Functions Prototype
 *--------------------*/
void	UART_SetBaudRate(volatile UART_REGS *baseAddr, UINT rate, UINT clk);
void	UART_SetLineCoding(volatile UART_REGS *baseAddr, LineCodingStru *pLC);
void	UART_GetLineCoding(volatile UART_REGS *baseAddr, LineCodingStru *pLC);
void	UART_SetXonXoffSymbol(volatile UART_REGS *baseAddr, UCHAR XonChar, UCHAR XoffChar);
void	UART_SetInterruptControl(volatile UART_REGS *baseAddr, InterruptControl *control);
void	UART_GetInterruptStatus(volatile UART_REGS *baseAddr, InterruptStatus *status);
void	UART_GetTransmitterStatus(volatile UART_REGS *baseAddr, UCHAR *status);
void	UART_SetTransmitterCommand(volatile UART_REGS *baseAddr, UCHAR cmd);
void	UART_SetTransmitterData(volatile UART_REGS *baseAddr, UINT count, UCHAR data);
void	UART_GetTransmitterDMAStatus(volatile UART_REGS *baseAddr, TransmitterDMAStatus *status);
void	UART_SetReceiverCommand(volatile UART_REGS *baseAddr, UCHAR cmd);
void	UART_GetReceiverData(volatile UART_REGS *baseAddr, UCHAR *rdata);
void	UART_GetReceiverStatusData(volatile UART_REGS *baseAddr, ReceiverStatusData *statusData);
void	UART_SetReceiverDMAWaterMark(volatile UART_REGS *baseAddr, UINT mark);
void	UART_GetReceiverDMAStatus(volatile UART_REGS *baseAddr, ReceiverDMAStatus *status);
void	UART_SetACIOControl(volatile UART_REGS *baseAddr, int idx, UCHAR mode, UCHAR source);
void	UART_GetACIOStatus(volatile UART_REGS *baseAddr, int idx, ACIOStatus *status);
