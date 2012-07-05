/*
 *  ROM info
 */
.equ ROM_BASE_ADDRESS,  0x1FC00004
/*.equ ROM_BASE_ADDRESS,  0x02000004*/


/*
 *  CLOCK
 */
.equ SYS_CLOCK, 0x1B000000
.equ SYS_CLOCK_CHG, 0x1B000008
.equ SYS_CLOCK_CFG3, 0x1B000014

/*
 *  CLOCK
 */
.equ INT_EM_0, 0x1B000100

/*
 *  NOR Flash direct access address
 */
.equ NOR_DIR_ADDRESS,	0x19800000

.equ BASE_NOR_FLASH,	0x19780000
.equ PIO_DATA_REG,	BASE_NOR_FLASH+0x00
.equ CONFIG_REG,	BASE_NOR_FLASH+0x04
.equ COM_REG,		BASE_NOR_FLASH+0x08
.equ INTR_S_REG,	BASE_NOR_FLASH+0x0C @ interrupt
.equ NOR_ADDR_REG,	BASE_NOR_FLASH+0x10
.equ CTRL_ST_REG,	BASE_NOR_FLASH+0x14 @ status
.equ INTR_M_REG,	BASE_NOR_FLASH+0x18
.equ SOFT_RST_REG,	BASE_NOR_FLASH+0x1C
.equ DMA_S_REG,		BASE_NOR_FLASH+0x20
.equ TIMER_REG,		BASE_NOR_FLASH+0x24
.equ PAD_REG,		BASE_NOR_FLASH+0x28
.equ PPI_PRO_CNT_REG,	BASE_NOR_FLASH+0x2C
@//.equ PPI_TIMING_REG,	BASE_NOR_FLASH+0x30
.equ ADDR_CFG1,		BASE_NOR_FLASH+0x34
.equ ADDR_CFG2,		BASE_NOR_FLASH+0x38
.equ ADDR_CFG3,		BASE_NOR_FLASH+0x3C
.equ ADDR_CFG4,		BASE_NOR_FLASH+0x40
.equ ADDR_CFG5,		BASE_NOR_FLASH+0x44
.equ DATA_CFG1,		BASE_NOR_FLASH+0x48 @ command for SPI
.equ DATA_CFG2,		BASE_NOR_FLASH+0x4C
.equ DATA_CFG3,		BASE_NOR_FLASH+0x50
.equ DATA_CFG4,		BASE_NOR_FLASH+0x54
.equ DATA_CFG5,		BASE_NOR_FLASH+0x58


/*
 *  SDRAM controller
 */
.equ Reg_1,       0x1C000000
.equ Reg_2,       0x1C000004


/*
 *  SHOW MESSAGE
 */
.equ def_DEBUG_PORT, 0x1B800080



/*
 *  console info
 *
 */

.equ CON_Status,  0x1B000400
.equ CON_Data,    0x1B000401
.equ CON_RST,     0x1B000402
.equ CON_GS,      0x1B000407


