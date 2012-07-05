#ifndef __TIME_STAMP_H__
#define __TIME_STAMP_H__

#include <asm/types.h>

typedef __u64 				stamp_t;

#define to_sec(xstamp)      ((int)((xstamp)/1000000000))
#define to_ms(xstamp)		((xstamp)/1000000)
#define to_us(xstamp)		((xstamp)/1000)


#define HOUR(xstamp)        ((int)(to_sec(xstamp))/3600)
#define MINUTE(xstamp)      (((int)(to_sec(xstamp))/60)%60)
#define SECOND(xstamp)      ((int)(to_sec(xstamp))%60)

#define rFTSTP_SEC          0xDB00022C
#define rFTSTP_FRAC         0xDB000228

extern stamp_t GetNSTimeStamp(void);
extern stamp_t Get90KTimeStamp(void);

#endif
