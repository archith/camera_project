#include <linux/ioctl.h>

typedef struct CacheTS_Dev {
   struct CacheTS_Dev *next;  /* next listitem */
} CacheTS_Dev;

#ifndef min
#  define min(a,b) ((a)<(b) ? (a) : (b))
#endif

/*
 * Ioctl definitions
 */

/* Use 'K' as magic number */
#define CACHETEST_IOC_MAGIC  'K'

#define CACHETEST_IOCOFFSET  				_IOW(CACHETEST_IOC_MAGIC, 0, quantum)
#define CACHETEST_IOCINVAILD  			_IOW(CACHETEST_IOC_MAGIC, 1, quantum)
#define CACHETEST_IOCINVAILDALIGN  	_IOW(CACHETEST_IOC_MAGIC, 2, quantum)
#define CACHETEST_IOCCLEAN	  			_IOW(CACHETEST_IOC_MAGIC, 3, quantum)
#define CACHETEST_IOCFLUSH	  			_IOW(CACHETEST_IOC_MAGIC, 4, quantum)
#define CACHETEST_IOCFLUSHCLEAN			_IOW(CACHETEST_IOC_MAGIC, 5, quantum)
#define CACHETEST_IOCFLUSHPAGE 			_IOW(CACHETEST_IOC_MAGIC, 6, quantum)
#define CACHETEST_IOCFLUSHBOUNDARY	_IOW(CACHETEST_IOC_MAGIC, 7, quantum)
#define CACHE_IOCICACHE							_IOW(CACHETEST_IOC_MAGIC, 8, quantum)

#define CACHETEST_IOC_MAXNR 8