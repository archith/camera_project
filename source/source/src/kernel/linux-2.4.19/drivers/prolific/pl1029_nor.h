
#ifndef rNOR_BASE
#error "No proper base register defined for PL-1029 NOR flash"
#endif

/* register definitions */
#define PIO_DATA_REG	(rNOR_BASE + 0x00)
#define CONFIG_REG	(rNOR_BASE + 0x04)
#define COM_REG		(rNOR_BASE + 0x08)
#define INTR_S_REG	(rNOR_BASE + 0x0c)
#define NOR_ADDR_REG	(rNOR_BASE + 0x10)
#define CTRL_ST_REG	(rNOR_BASE + 0x14)
#define INTR_M_REG	(rNOR_BASE + 0x18)
#define SOFT_RST_REG	(rNOR_BASE + 0x1c)
#define DMA_S_REG	(rNOR_BASE + 0x20)
#define TIMER_REG	(rNOR_BASE + 0x24)
#define PAD_REG		(rNOR_BASE + 0x28)
#define PPI_PRO_CNT_REG	(rNOR_BASE + 0x2c)
#define ADDR_CFG1	(rNOR_BASE + 0x34)
#define ADDR_CFG2	(rNOR_BASE + 0x38)
#define ADDR_CFG3	(rNOR_BASE + 0x3c)
#define ADDR_CFG4	(rNOR_BASE + 0x40)
#define ADDR_CFG5	(rNOR_BASE + 0x44)
#define DATA_CFG1	(rNOR_BASE + 0x48)
#define DATA_CFG2	(rNOR_BASE + 0x4c)
#define DATA_CFG3	(rNOR_BASE + 0x50)
#define DATA_CFG4	(rNOR_BASE + 0x54)
#define DATA_CFG5	(rNOR_BASE + 0x58)

/* taginfo passed by Proboot, it's the nor flash mapping table */
#define MAX_SECTOR_SET	16
struct tag_norinfo {
        u32	type;
        struct tag_sectinfo {
                u16	size;
                u16	quantum;
        } sectinfo[MAX_SECTOR_SET];
};
#define ATAG_NORINFO 0x10290101

typedef struct {
	unsigned long start_addr;
	unsigned int size;
} nor_sector_t;
#define MAX_NOR_SECTOR	1024

#define DMA_START	1
#define DMA_END		2

typedef struct {
	int (*init)(void);
	int (*reset)(void);
	int (*read_id)(u8 *);
	int (*cmd_erase)(unsigned long, int);
	int (*cmd_write)(unsigned long, int, int);
	int (*cmd_read)(unsigned long, int, int);
} pl1029_nor_ops;

struct norflash {
	int type;
	pl1029_nor_ops *ops;
};

#define NOR_TIMEOUT_INTR_MASK	(1<<3)
#define NOR_CMD_INTR_MASK	(1<<2)
#define NOR_DMA_INTR_MASK	(1<<0)

#define NOR_INTR_MASKALL	(NOR_TIMEOUT_INTR_MASK|NOR_CMD_INTR_MASK|NOR_DMA_INTR_MASK)
#define NOR_INTR_SET_MASK(m)	\
do {				\
	unsigned char mask;	\
	mask = 0x00;		\
	mask |= m;		\
	writeb(mask, INTR_M_REG);\
} while(0)


