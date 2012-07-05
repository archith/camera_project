#ifndef __TIMER_H__
#define __TIMER_H__

#define rRTC	0x1b000204

#include <io.h>
#define CURRENT	(readl (rRTC))

#endif // __TIMER_H__
