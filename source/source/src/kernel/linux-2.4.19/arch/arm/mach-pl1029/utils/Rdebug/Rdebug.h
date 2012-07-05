#include <linux/ioctl.h>

/* version dependencies have been confined to a separate file */

/*
 * Macros to help debugging
 */

#define REGDEBUG_MAJOR 0  

#define REGDEBUG_DEVS 4   

#define REGDEBUG_QUANTUM  4000 /* use a quantum size like scull */
#define REGDEBUG_QSET     500

typedef struct Regdebug_Dev {
   u32 *real_uncached;
   dma_addr_t dma_handle;
   struct Regdebug_Dev *next;  /* next listitem */
   int vmas;                 /* active mappings */
   int quantum;              /* the current allocation size */
   int qset;                 /* the current array size */
   size_t size;              /* 32-bit will suffice */
   struct semaphore sem;     /* Mutual exclusion */
} Regdebug_Dev;

extern Regdebug_Dev *regdebug_devices;

extern struct file_operations regdebug_fops;

/*
 * The different configurable parameters
 */
extern int regdebug_major;     /* main.c */
extern int regdebug_devs;


/*
 * Prototypes for shared functions
 */
int regdebug_trim(Regdebug_Dev *dev);

#ifndef min
#  define min(a,b) ((a)<(b) ? (a) : (b))
#endif

/*
 * Ioctl definitions
 */

/* Use 'K' as magic number */
#define REGDEBUG_IOC_MAGIC  'K'

#define REGDEBUG_IOCRESET                   _IO(REGDEBUG_IOC_MAGIC, 0) 

#define REGDEBUG_REG_READ_VIRT_B            _IOW(REGDEBUG_IOC_MAGIC,   1, regdebug_address)
#define REGDEBUG_REG_READ_PHY_B             _IOW(REGDEBUG_IOC_MAGIC,   2, regdebug_address)
#define REGDEBUG_REG_WRITE_VIRT_B           _IOW(REGDEBUG_IOC_MAGIC,   3, regdebug_address)
#define REGDEBUG_REG_WRITE_PHY_B            _IOW(REGDEBUG_IOC_MAGIC,   4, regdebug_address)
#define REGDEBUG_REG_READ_VIRT_W            _IOW(REGDEBUG_IOC_MAGIC,   5, regdebug_address)
#define REGDEBUG_REG_READ_PHY_W             _IOW(REGDEBUG_IOC_MAGIC,   6, regdebug_address)
#define REGDEBUG_REG_WRITE_VIRT_W           _IOW(REGDEBUG_IOC_MAGIC,   7, regdebug_address)
#define REGDEBUG_REG_WRITE_PHY_W            _IOW(REGDEBUG_IOC_MAGIC,   8, regdebug_address)

#define REGDEBUG_USER_READ_VIRT_B           _IOW(REGDEBUG_IOC_MAGIC,   9, regdebug_address)
#define REGDEBUG_USER_READ_PHY_B            _IOW(REGDEBUG_IOC_MAGIC,  10, regdebug_address)
#define REGDEBUG_USER_WRITE_VIRT_B          _IOW(REGDEBUG_IOC_MAGIC,  11, regdebug_address)
#define REGDEBUG_USER_WRITE_PHY_B           _IOW(REGDEBUG_IOC_MAGIC,  12, regdebug_address)
#define REGDEBUG_USER_READ_VIRT_W           _IOW(REGDEBUG_IOC_MAGIC,  13, regdebug_address)
#define REGDEBUG_USER_READ_PHY_W            _IOW(REGDEBUG_IOC_MAGIC,  14, regdebug_address)
#define REGDEBUG_USER_WRITE_VIRT_W          _IOW(REGDEBUG_IOC_MAGIC,  15, regdebug_address)
#define REGDEBUG_USER_WRITE_PHY_W           _IOW(REGDEBUG_IOC_MAGIC,  16, regdebug_address)

#define REGDEBUG_IODEFAULT                 _IOW(REGDEBUG_IOC_MAGIC,   17, regdebug_address)

#define REGDEBUG_IOC_MAXNR 17



