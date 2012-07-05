/*
 * asm/arch-armnommu/arch-pl1097/dma.h:
 *
 */

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H


#define MAX_DMA_ADDRESS   0xffffffff
#define MAX_DMA_CHANNELS  16

/* PL1029 DMA Channel definition */
#define DMA_PL_SM_DATA      0
#define DMA_PL_SM_RDNT      1
#define DMA_PL_AC97_OUT     2
#define DMA_PL_AC97_IN      3
#define DMA_PL_ADC          4       /* ADC removed */
#define DMA_PL_IDE          4
#define DMA_PL_SD           5
#define DMA_PL_CF           6       /* CF removed in 1029 */
#define DMA_PL_UART_1_IN    6
#define DMA_PL_UART_OUT     7
#define DMA_PL_UART_IN      8
#define DMA_PL_MS           9
#define DMA_PL_UART_1_OUT   10
#define DMA_PL_SPI          11
#define DMA_PL_AES_UP       12
#define DMA_PL_AES_DN       13
#define DMA_PL_NOR          14

/* PL1029 DMA register define */
#define DMA_BURST_LINE      (1 << 3)

/* prolific special functions */
extern void set_dma_mde_base(dmach_t channel, const char *mde_vaddr);
extern void set_dma_mde_idx(dmach_t channel, unsigned int mde_idx);
extern void set_dma_burst_length(dmach_t channel, char burst);



#endif /* _ASM_ARCH_DMA_H */
