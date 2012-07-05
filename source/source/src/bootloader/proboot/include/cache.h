#ifndef __CACHE_H__
#define __CACHE_H__

#if CONFIG_CACHE_ON
#define cache_on()      __cache_on(0)
#define cache_off()     __cache_off(0)
#define cache_flush()   __cache_flush(0)

extern void __cache_on(int a);
extern void __cache_off(int a);
extern void __cache_flush(int a);
#else
#define cache_on()
#define cache_off()
#define cache_flush()
#endif

#define cache_status()  __cache_status(0)
extern int __cache_status(int a);
#define HIGH_VECTOR	1<<13	//V, vector
#define IC_ENABLE	1<<12	//I, i cache
#define BTB_ENABLE	1<<11	//B, branch
#define ROM_PROTECTION	1<<9	//R, rom
#define SYS_PROTECTION	1<<8	//S, system
#define BIG_ENDIAN	1<<7	//B/L, endian
#define WB_ENABLE	1<<3	//W, write buffer
#define DC_ENABLE	1<<2	//C, d cache
#define ALIGNMENT	1<<1	//A, alignment
#define MMU_ENABLE	1<<0	//M, MMU



#endif /* __CACHE_H__ */
