#include <asm/types.h>
//typedef unsigned long long 	u64;
typedef __u64 				stamp_t;

// to print the __u64 value, use %llu

// give a timestamp calculate how many hours, minutes and seconds
#define to_sec(xstamp)      ((int)(xstamp/1000000000))

#define HOUR(xstamp)        ((int)(to_sec(xstamp))%3600)
#define MINUTE(xstamp)      (((int)(to_sec(xstamp))/3600)%60)
#define SECOND(xstamp)      ((int)(to_sec(xstamp))%60)

#define rFTSTP_SEC          0xDB00022C
#define rFTSTP_FRAC         0xDB000228

static stamp_t stamp_mark(void)
{
    __u32   sec;
    __u32   frac;
    stamp_t stamp;

    sec = readl(rFTSTP_SEC);
    frac = readl(rFTSTP_FRAC);
    if ((frac & (1 << 31)) == 0)
        sec = readl(rFTSTP_SEC);

    stamp = (stamp_t) sec * 1000000000 +
            ((stamp_t)frac * 1953125 + (1 << 22)) / (1 << 23);
    return stamp;

}
/*
static stamp_t Get90KTimeStamp(void)
{
    __u32   sec;
    __u32   frac;
    stamp_t stamp;

    sec = readl(rFTSTP_SEC);
    frac = readl(rFTSTP_FRAC);
    if ((frac & (1 << 31)) == 0)
        sec = readl(rFTSTP_SEC);

    stamp = (stamp_t) sec * 1000000000 +
            ((stamp_t)frac * 1953125 + (1 << 22)) / (1 << 23);
    return stamp;

}
*/
