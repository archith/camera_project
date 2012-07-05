#ifndef __STDIO_H__
#define __STDIO_H__

#define NULL		0

#define ESC		0x1b
#define BKT		0x5b
#define WAV		0x7e
#define BACKSPACE	0x08

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

#define STR_CNT		8
#define STR_SIZE	1024

void putchar (int);
int getchar (void);

char *gets (char *buf);
void puts (char *buf);

int printf (const char *fmt, ...);

//#define DEBUG
#ifdef DEBUG
#define dprintf(fmt, arg...)    printf(fmt, ##arg)
#define MSG_DBG(fmt, ...) \
printf ("[1;34m%s:%d[0m %s: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define MSG_ERR(fmt, ...) \
printf ("[1;31m%s:%d[0m %s: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define dprintf(fmt, arg...)
#define MSG_DBG(fmt, ...)
#define MSG_ERR(fmt, ...) printf (fmt, ##__VA_ARGS__)
#endif

#endif
