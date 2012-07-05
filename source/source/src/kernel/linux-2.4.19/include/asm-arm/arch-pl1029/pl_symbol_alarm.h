#ifndef __PL_SYMBOL_ALARM_H__
#define __PL_SYMBOL_ALARM_H__

#include <linux/ioctl.h>

typedef struct
{
	int	nPID;
	int	nFrequency;			//	Hz
	int	nMaxBufferSize;
	int	nMaxBufferAmount;
}	PLSymbolAlarm, *PPLSymbolAlarm;

#define SNDCTL_DSP_GETIDELAY		_SIOR ('P', 24, int)

#define PL_SNDCTL_DSP_SYMALR_REQ		_IOWR('P', 70, PLSymbolAlarm)
#define PL_SNDCTL_DSP_SYMALR_ACTIVE		_IOWR('P', 71, int)
	#define PL_SYMBOL_ALARM_QUERY		-1
	#define PL_SYMBOL_ALARM_DISABLE		0
	#define PL_SYMBOL_ALARM_ENABLE		1
#define PL_SNDCTL_DSP_SYMALR_REL		_IO('P', 72)
#define PL_SNDCTL_DSP_SYMALR_FREQUENCY  	_IOWR('P', 73, int)

#define PL_POST_CLEAN					1
#define PL_POST_KEEP					0


#endif
