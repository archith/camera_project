#ifndef __SERIAL_H__
#define __SERIAL_H__

#define SERIAL_REG_BASE	0x1b000400
#define SERIAL_REG_ST	(SERIAL_REG_BASE + 0)
#define SERIAL_REG_DATA	(SERIAL_REG_BASE + 1)
#define SERIAL_REG_RST	(SERIAL_REG_BASE + 2)
#define SERIAL_REG_SRB	(SERIAL_REG_BASE + 3)
#define SERIAL_REG_PL	(SERIAL_REG_BASE + 4)
#define SERIAL_REG_PH	(SERIAL_REG_BASE + 5)
#define SERIAL_REG_CMP	(SERIAL_REG_BASE + 6)
#define SERIAL_REG_GS	(SERIAL_REG_BASE + 7)

#define INBOUND_READY	(1 << 7)
#define ERR_OVERRUN	(1 << 6)
#define ERR_PARITY	(1 << 5)
#define ERR_FRAME	(1 << 4)
#define SERIAL_BREAK	(1 << 3)
#define OUTBOUND_EMPTY	(1 << 2)
#define OUTBOUND_FULL	(1 << 1)
#define CTS_STATUS	(1 << 0)

#define DET_EN		(1 << 7) // Baud Rate Auto Detection Enable

void putb (char byte);
int getb (void);
int getb2 (void);

int console_exist (void);
int console_detect (void);


#endif // __SERIAL_H__

