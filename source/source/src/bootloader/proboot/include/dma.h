#ifndef __DMA_H__
#define __DMA_H__

/* DMA controller registers */
//#define _1029_fpga
#ifndef _1029_fpga
#define DMA_REG_BASE	0x197c0000
#else
#define DMA_REG_BASE	0x1a3C0000
#endif //_1029_fpga
/* DMA controller registers */
#define DMA_IDX_REG         0x00    /* MDE index register */
#define DMA_DIR_REG         0x02    /* Count direction. Cont down (1), Count up (0)  */
#define DMA_CMD_REG         0x03    /* Channel command register */
#define DMA_REG_CHN	(DMA_REG_BASE + 0x00)
#define DMA_REG_MDT	(DMA_REG_BASE + 0x40)

/* PL1063 DMA Channel definition */
#define DMA_PL_SM_DATA      0
#define DMA_PL_SM_RDNT      1
#define DMA_PL_AC97_OUT     2
#define DMA_PL_AC97_IN      3
#define DMA_PL_ADC          4
#define DMA_PL_SD           5
#define DMA_PL_CF           6
#define DMA_PL_UART_OUT     7
#define DMA_PL_UART_IN      8
#define DMA_PL_MS           9
#define DMA_PL_NORFLASH	   14
#define DMA_CHN0	0
#define DMA_CHN1	1
#define DN_STREAM	0
#define UP_STREAM	1

struct dma_mdt
{
	unsigned int saddr;
	unsigned int mde;
};

void setup_dma (int channel, int mode, void *buffer, int len);
int isDMACActive(void);
void disable_DMAC(void);

#ifndef ALIGN4
#define ALIGN4(x)   (((unsigned int)(x) + 3) & (-4))
#endif

#define ROUND_TO_LINE(val)	(((val)+15)&-16)
#define LINE_ALIGN(xaddr)	((((unsigned int)(xaddr))%16)                   \
                             	? ROUND_TO_LINE(((unsigned int)(xaddr)))    \
                             	: ((unsigned int)(xaddr)))
                             	
#endif // __DMA_H__
