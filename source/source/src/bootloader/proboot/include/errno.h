

/* 0x00:0x0F - unused (reserved for the user defined values) */

/* 0x10:0x1F - SmartMedia Error Code */
#define ST_MTD_UUB			0x10	/* Unused block */

#define ERR_SM_INIT			0x11	/* SmartMedia initialization error */
#define ERR_SM_GET_PHY_ADDR		0x12	/* can not find the physical block by the logic one */
#define ERR_SM_READ_PAGE		0x13	/* access page error */

#define ERR_MTD_IDS			0x14	/* invalid device selector */
#define ERR_MTD_IDT			0x15	/* invalid device type */
#if 0
#define ERR_MTD_ILBN			0x16	/* invalid logical block number (access out of boundary) */
#define ERR_MTD_IPBN			0x17	/* invalid physical block number (access out of boundary) */
#define ERR_MTD_IDL			0x18	/* invalid data length */
#else
#define ERR_MTD_BD			0x16	/* bad data */
#endif
#define ERR_MTD_BB			0x19	/* bad block */
#define ERR_MTD_WP			0x1A	/* write protection */
#define ERR_MTD_MA			0x1B	/* memory address alignment error */
#define ERR_MTD_PC			0x1C	/* parity error */
#define ERR_MTD_ECC			0x1D	/* data transmission error */
#define ERR_MTD_PRG			0x1E	/* programming error */

/* 0x20:0x2F - DMA Error Code */
#define ERR_DMA_IC			0x21	/* invalid channel number */
#define ERR_DMA_IM			0x22	/* invalid mode */
#define ERR_DMA_ID			0x23	/* invalid DMA transfer direction */
#define ERR_DMA_MA			0x25	/* memory address alignment error */
#define ERR_DMA_LL			0x26	/* length too long to be handled by DMA access */
#define ERR_DMA_LOCK			0x27	/* DMA channel locked */

/* 0x30:0x3F - Console Error/Status Code */
#define ST_CONSOLE_DETECTING		0x31	/* detecting console */
#define ST_CONSOLE_IDR			0x32	/* input data fifo ready */
#define ST_CONSOLE_ODE			0x33	/* output data fifo empty */
#define ST_CONSOLE_ODF			0x34	/* output data fifo full */
#define ERR_CONSOLE_OE			0x35	/* overrun error */
#define ERR_CONSOLE_PE			0x36	/* parity error */
#define ERR_CONSOLE_FE			0x37	/* frame error */
#define ERR_CONSOLE_SB			0x38	/* serial break */
#define ERR_CONSOLE_BD			0x39	/* baud rate detection error (serial absent or baud rate too high) */
#define ERR_CONSOLE_CA			0x3A	/* serial absent */

/* 0x40:0x5F - Memory Access Error Code */
#define ERR_MEM_SAF0			0x41	/* march error: SAF0 */
#define ERR_MEM_SAF1			0x42	/* march error: SAF1 */
#define ERR_MEM_CFin0			0x43	/* march error: CFin0 */
#define ERR_MEM_CFin1			0x44	/* march error: CFin1 */
#define ERR_MEM_CFin2			0x45	/* march error: CFin2 */
#define ERR_MEM_CFin3			0x46	/* march error: CFin3 */
#define ERR_MEM_CFst0			0x47	/* march error: CFst0 */
#define ERR_MEM_CFst1			0x48	/* march error: CFst1 */
#define ERR_MEM_CFst2			0x49	/* march error: CFst2 */
#define ERR_MEM_CFst3			0x4A	/* march error: CFst3 */
#define ERR_MEM_CFst4			0x4B	/* march error: CFst4 */
#define ERR_MEM_CFst5			0x4C	/* march error: CFst5 */
#define ERR_MEM_CFst6			0x4D	/* march error: CFst6 */
#define ERR_MEM_CFst7			0x4E	/* march error: CFst7 */
#define ERR_MEM_OTHERS			0x4F	/* march error: others */

#define ERR_MEM_ADEL			0x51	/* address error (load) */
#define ERR_MEM_ADES			0x52	/* address error (store) */
#define ERR_MEM_DB			0x53	/* data bus error */
#define ERR_MEM_AB			0x54	/* address bus error */
#define ERR_MEM_SELF			0x55	/* CRC32 self test error */
#define ERR_MEM_MARCH			0x56	/* march test error */

/* 0x60:0x6F - NOR Error Code */
#define ERR_NOR_INIT			0x60	/* nor flash init error */
#define ERR_NOR_DMA			0x61	/* nor flash diag DMA error */
#define ERR_NOR_PIO			0x62	/* nor flash diag PIO error */
#define ERR_NOR_ERASE			0x63	/* nor flash diag Erase error */
#define ERR_NOR_CMP0			0x64	/* nor flash diag Compare error 0 */
#define ERR_NOR_CMP1			0x65	/* nor flash diag Compare error 1 */
#define ERR_NOR_CMP2			0x66	/* nor flash diag Compare error 2 */
#define ERR_NOR_CMP3			0x67	/* nor flash diag Compare error 3 */
#define ERR_NOR_CMP4			0x68	/* nor flash diag Compare error 4 */
#define ERR_NOR_CMP5			0x69	/* nor flash diag Compare error 5 */

/* 0x70:0x7F - tftp&M2M Error Code */
#define ERR_FTP_CMP			0x70	/* tftp Compare error */
#define ERR_M2M_BUSY			0x71	/* M2M Channel Busy */
#define ERR_M2M_CMP			0x72	/* M2M Compare error */

/* 0xA0:0xAF - FIS Error/Status Code */
#define ERR_FIS_IS			0xA1	/* illegal accessible scope */
#define ERR_FIS_WD			0xA2	/* damage ProBoot in writing */
#define ERR_FIS_NP			0xA3	/* no more unused physical block */
#define ERR_FIS_NL			0xA4	/* no requested logical block */
#define ERR_FIS_ADDR			0xA5	/* invalid memory address */
#define ERR_FIS_LEN			0xA6	/* invalid data length */
#define ERR_FIS_OOL			0xA7	/* out of logical accessible scope */
#define ERR_FIS_OOB			0xA8	/* out of region reserved for bootloader */

/* 0xB0:0xBF - Download Error/Status Code */
#define ERR_DOWNLOAD_XMOD_ESC		0xB1	/* cancled by user */
#define ERR_DOWNLOAD_XMOD_NR		0xB2	/* no response from sender */

/* 0xC0:0xCF - ProBoot Error/Status Code */
#define ST_PROBOOT_INSTALL		0xC1	/* ProBoot is installed successfully */
#define ERR_PROBOOT_INSTALL		0xC2	/* ProBoot is installed incompletely */
#define ERR_PROBOOT_CMD			0xC3	/* ProBoot command error */
#define ERR_PROBOOT_CNF			0xC4	/* ProBoot command not found */



/* 0xD0:0xDF - System Exception (reserved for the ROM code) */
#define ERR_EXCEPTION_INT		0xD0	/* interrupt */
#define ERR_EXCEPTION_MOD		0xD1	/* TLB modification */
#define ERR_EXCEPTION_TLBL		0xD2	/* TLB load */
#define ERR_EXCEPTION_TLBS		0xD3	/* TLB store */
#define ERR_EXCEPTION_ADEL		0xD4	/* address error (load) */
#define ERR_EXCEPTION_ADES		0xD5	/* address errro (store) */
#define ERR_EXCEPTION_IBE		0xD6	/* bus error (instruction) */
#define ERR_EXCEPTION_DBE		0xD7	/* bus error (data) */
#define ERR_EXCEPTION_SYSCALL		0xD8	/* generated unconditionally by a syscall instruction */
#define ERR_EXCEPTION_BP		0xD9	/* breakpoint */
#define ERR_EXCEPTION_RI		0xDA	/* reserved instruction */
#define ERR_EXCEPTION_CPU		0xDB	/* co-processor unusable */
#define ERR_EXCEPTION_OV		0xDC	/* arithmetic overflow */
#define ERR_EXCEPTION_TR		0xDD	/* trap exception */
#define ERR_EXCEPTION_FPE		0xDF	/* floating-point exception */

