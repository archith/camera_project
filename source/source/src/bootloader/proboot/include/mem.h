#ifndef __MEM_H__
#define __MEM_H__

#define CMC_REG_BASE	0x1c000000
#define CMC_REG_MMAP	(CMC_REG_BASE + 0x00)
#define CMC_REG_MMOD	(CMC_REG_BASE + 0x04)

/* Memory to memory copy DMA Control registers 0x1c000080-0x1c00009c */
#define M2M_REG_BASE	0x1c000080
#define M2M_REG_SRC1	(M2M_REG_BASE + 0x00)
#define M2M_REG_DEST1	(M2M_REG_BASE + 0x04)
#define M2M_REG_START1	(M2M_REG_BASE + 0x08)
#define M2M_REG_STAT1	(M2M_REG_BASE + 0x0c)
#define M2M_REG_SRC2	(M2M_REG_BASE + 0x10)
#define M2M_REG_DEST2	(M2M_REG_BASE + 0x14)
#define M2M_REG_START2	(M2M_REG_BASE + 0x18)
#define M2M_REG_STAT2	(M2M_REG_BASE + 0x1c)

#include <io.h>
#define MEMSIZE		(readw (CMC_REG_MMAP + 2) << 16)

#endif // __MEM_H__
