#ifndef __VT100_H__
#define __VT100_H__

#define NUL		0x00
#define SOH		0x01
#define STX		0x02
#define ETX		0x03
#define EOT		0x04
#define ENQ		0x05
#define ACK		0x06
#define BEL		0x07
#define BS		0x08
#define HT		0x09
#define NL		0x0a
#define VT		0x0b
#define NP		0x0c
#define CR		0x0d
#define SO		0x0e
#define SI		0x0f
#define DLE		0x10
#define DC1		0x11
#define DC2		0x12
#define DC3		0x13
#define DC4		0x14
#define NAK		0x15
#define SYN		0x16
#define ETB		0x17
#define CAN		0x18
#define EM		0x19
#define SUB		0x1a
#define ESC		0x1b
#define FS		0x1c
#define GS		0x1d
#define RS		0x1e
#define US		0x1f

#define BKT		0x5b
#define WAV		0x7e

#define CURSOR_UP	(256 + 0)	// (ESC, BKT, 'A')
#define CURSOR_DOWN	(256 + 1)	// (ESC, BKT, 'A')
#define CURSOR_RIGHT	(256 + 2)	// (ESC, BKT, 'C')
#define CURSOR_LEFT	(256 + 3)	// (ESC, BKT, 'D')
#define HOME		(256 + 4)	// (ESC, BKT, '1', '~')
#define INSERT		(256 + 5)	// (ESC, BKT, '2', '~')
#define DELETE		(256 + 6)	// (ESC, BKT, '3', '~')
#define END		(256 + 7)	// (ESC, BKT, '4', '~')
#define PAGE_DOWN	(256 + 8)	// (ESC, BKT, '5', '~')
#define PAGE_UP		(256 + 9)	// (ESC, BKT, '6', '~')

int getkey (void);
char *getstr (void);

#endif // __VT100_H__
