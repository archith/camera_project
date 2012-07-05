
#ifndef __CACHE_MEMORY_CONTROLLER__
#define __CACHE_MEMORY_CONTROLLER__


/*********************************************************************************
*
*       Control Register Address Definiton
*
*********************************************************************************/
/*
 *  PL1060 Memory Mapping Definition
 *  by Jim Lee
 *  2001/11/14
 */

#define CACHE_START                     0xBD000000
#define CACHE_END                       0xBD004800

/* Memory mode -- follow the programming guide released at 15/Mar/2001 */

#define RAM_DATA_BUS_WIDTH_SHIFT_BIT          0
#define ROM_DATA_BUS_WIDTH_SHIFT_BIT          2
#define ACCESS_RECOVERY_TIME_SHIFT_BIT        4
#define RAM_TYPE_SHIFT_BIT                   12
#define ROM_ACCESS_TIME_SHIFT_BIT            16
#define FLASH_ACCEESS_TIME_SHIFT_BIT         21
#define RAM_ACCESS_TIME_SHIFT_BIT            26

#define RAM_DATA_BUS_32_BIT          ( 0b00 << RAM_DATA_BUS_WIDTH_SHIFT_BIT )
#define RAM_DATA_BUS_16_BIT          ( 0b10 << RAM_DATA_BUS_WIDTH_SHIFT_BIT )
#define RAM_DATA_BUS_08_BIT          ( 0b11 << RAM_DATA_BUS_WIDTH_SHIFT_BIT )

#define ROM_DATA_BUS_32_BIT          ( 0b00 << ROM_DATA_BUS_WIDTH_SHIFT_BIT )
#define ROM_DATA_BUS_16_BIT          ( 0b10 << ROM_DATA_BUS_WIDTH_SHIFT_BIT )
#define ROM_DATA_BUS_08_BIT          ( 0b11 << ROM_DATA_BUS_WIDTH_SHIFT_BIT )

#define Ext_SDRAM                    ( 0b00 << RAM_TYPE_SHIFT_BIT )
#define Ext_SRAM                     ( 0b10 << RAM_TYPE_SHIFT_BIT )
#define Ext_PBSRAM                   ( 0b11 << RAM_TYPE_SHIFT_BIT )


#define DEFAULT_MODE                 0x412A0700

#define MEMORY_MODE                  ( DEFAULT_MODE + RAM_DATA_BUS_32_BIT + ROM_DATA_BUS_08_BIT + Ext_SDRAM )


/* Memory map -- follow the programming guide released at 15/Mar/2001 */

#define KERNEL_SIZE_SHIFT_BIT              0
#define RAM_SIZE_SHIFT_BIT                 4
#define FRAME_BUFFER_SIZE_SHIFT_BIT        8

#define KERNEL_032K                        0
#define KERNEL_064K                        0
#define KERNEL_128K                        0
#define KERNEL_256K                        0
#define KERNEL_512K                        0
#define KERNEL_001M                        0
#define KERNEL_002M                        0
#define KERNEL_004M                        0
#define KERNEL_008M                        1
#define KERNEL_016M                        0
#define KERNEL_032M                        0
#define KERNEL_064M                        0

#define RAM_032K                           0
#define RAM_064K                           0
#define RAM_128K                           0
#define RAM_256K                           0
#define RAM_512K                           0
#define RAM_001M                           0
#define RAM_002M                           0
#define RAM_004M                           0

#define RAM_008M                           0
#define RAM_016M                           1

#define RAM_032M                           0
#define RAM_064M                           0

#define FB_004K                            0
#define FB_008K                            0
#define FB_016K                            0
#define FB_032K                            0
#define FB_064K                            0
#define FB_128K                            0
#define FB_256K                            0
#define FB_512K                            0
#define FB_001M                            1
#define FB_002M                            0
#define FB_004M                            0
#define FB_008M                            0
#define FB_016M                            0
#define FB_032M                            0
#define FB_064M                            0


#if KERNEL_032K
        #define KERNEL_SIZE_CONFIG    ( 0b0100 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00008000 )
#endif
#if KERNEL_064K
        #define KERNEL_SIZE_CONFIG    ( 0b0101 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00010000 )
#endif
#if KERNEL_128K
        #define KERNEL_SIZE_CONFIG    ( 0b0110 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00020000 )
#endif
#if KERNEL_256K
        #define KERNEL_SIZE_CONFIG    ( 0b0111 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00040000 )
#endif
#if KERNEL_512K
        #define KERNEL_SIZE_CONFIG    ( 0b1000 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00080000 )
#endif
#if KERNEL_001M
        #define KERNEL_SIZE_CONFIG    ( 0b1001 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00100000 )
#endif
#if KERNEL_002M
        #define KERNEL_SIZE_CONFIG    ( 0b1010 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00200000 )
#endif
#if KERNEL_004M
        #define KERNEL_SIZE_CONFIG    ( 0b1011 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00400000 )
#endif
#if KERNEL_008M
        #define KERNEL_SIZE_CONFIG    ( 0b1100 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x00800000 )
#endif
#if KERNEL_016M
        #define KERNEL_SIZE_CONFIG    ( 0b1101 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x01000000 )
#endif
#if KERNEL_032M
        #define KERNEL_SIZE_CONFIG    ( 0b1110 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x02000000 )
#endif
#if KERNEL_064M
        #define KERNEL_SIZE_CONFIG    ( 0b1111 << KERNEL_SIZE_SHIFT_BIT )
        #define KERNEL_SIZE           ( 0x04000000 )
#endif


#if RAM_032K
        #define RAM_SIZE_CONFIG       ( 0b0100 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00008000 )
#endif
#if RAM_064K
        #define RAM_SIZE_CONFIG       ( 0b0101 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00010000 )
#endif
#if RAM_128K
        #define RAM_SIZE_CONFIG       ( 0b0110 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE               ( 0x00020000 )
#endif
#if RAM_256K
        #define RAM_SIZE_CONFIG       ( 0b0111 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00040000 )
#endif
#if RAM_512K
        #define RAM_SIZE_CONFIG       ( 0b1000 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00080000 )
#endif
#if RAM_001M
        #define RAM_SIZE_CONFIG       ( 0b1001 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00100000 )
#endif
#if RAM_002M
        #define RAM_SIZE_CONFIG       ( 0b1010 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00200000 )
#endif
#if RAM_004M
        #define RAM_SIZE_CONFIG       ( 0b1011 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00400000 )
#endif
#if RAM_008M
        #define RAM_SIZE_CONFIG       ( 0b1100 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x00800000 )
#endif
#if RAM_016M
        #define RAM_SIZE_CONFIG       ( 0b1101 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x01000000 )
#endif
#if RAM_032M
        #define RAM_SIZE_CONFIG       ( 0b1110 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x02000000 )
#endif
#if RAM_064M
        #define RAM_SIZE_CONFIG       ( 0b1111 << RAM_SIZE_SHIFT_BIT )
        #define RAM_SIZE              ( 0x04000000 )
#endif



#if FB_004K
        #define FB_SIZE_CONFIG        ( 0b0000 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00001000 )    
#endif
#if FB_008K
        #define FB_SIZE_CONFIG        ( 0b0001 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00002000 )
#endif
#if FB_016K
        #define FB_SIZE_CONFIG        ( 0b0010 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00004000 )        
#endif
#if FB_032K
        #define FB_SIZE_CONFIG        ( 0b0011 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00008000 )
#endif
#if FB_064K
        #define FB_SIZE_CONFIG        ( 0b0100 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00010000 )
#endif
#if FB_128K
        #define FB_SIZE_CONFIG        ( 0b0101 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00020000 )
#endif
#if FB_256K
        #define FB_SIZE_CONFIG        ( 0b0110 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00040000 )
#endif
#if FB_512K
        #define FB_SIZE_CONFIG        ( 0b0111 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00080000 )
#endif
#if FB_001M
        #define FB_SIZE_CONFIG        ( 0b1000 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00100000 )
#endif
#if FB_002M
        #define FB_SIZE_CONFIG        ( 0b1001 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00200000 )
#endif
#if FB_004M
        #define FB_SIZE_CONFIG        ( 0b1010 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00400000 )
#endif
#if FB_008M
        #define FB_SIZE_CONFIG        ( 0b1011 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x00800000 )
#endif
#if FB_016M
        #define FB_SIZE_CONFIG        ( 0b1100 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x01000000 )
#endif
#if FB_032M
        #define FB_SIZE_CONFIG        ( 0b1101 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x02000000 )
#endif
#if FB_064M
        #define FB_SIZE_CONFIG        ( 0b1110 << FRAME_BUFFER_SIZE_SHIFT_BIT )
        #define FB_SIZE               ( 0x04000000 )
#endif



#define KERNEL_BASE                   ( 0x80000000 )
#define KERNEL_END                    ( KERNEL_BASE + KERNEL_SIZE )

#define FB_BASE                       ( 0x00000000 + RAM_SIZE - FB_SIZE )
#define FB_END                        ( 0x00000000 + RAM_SIZE )

#define LCD_FRAME_BASE                ( FB_BASE | 0xA0000000 )
#define LCD_FRAME_END                 ( FB_END | 0xA0000000  )

#define USER_BASE                     ( 0x00000000 + KERNEL_SIZE )
#define USER_END                      ( FB_BASE )






/* Memory Control */

#define RAM_CLOCK_TUNING_SHIFT_BIT            		 0
#define DATA_PATH_SAMPLING_CLOCK_SHIFT_BIT    		 4
#define DELAY_FOR_RAM_SHIFT_BIT               		 5
#define HALF_CYCLE_FOR_ROM_SHIFT_BIT          		 6
#define HALF_CYCLE_FOR_SRAM_SHIFT_BIT        		 7
#define REFRESH_DISTANCE_SHIFY_BIT           		24
#define REFRESH_CYCLES_PER_REQUEST_SHIFT_BIT	 	28



#define B_CLK                        ( 0b0 << DATA_PATH_SAMPLING_CLOCK_SHIFT_BIT )      /* for PBSRAM */
#define P_CLK                        ( 0b1 << DATA_PATH_SAMPLING_CLOCK_SHIFT_BIT )      /* for SRAM, SDRAM */
#define DEFAULT_CONTROL               0x2A00FF10

/* #define MEMORY_CONTROL                ( DEFAULT_CONTROL + P_CLK ) */
#define MEMORY_CONTROL		      0x2a00ff17	/* remove jumper 90/10/08 by Vincent PCI fixed to 66MHz */
/* SDRAM config */

#define SDRAM_CONFIG_VALUE            0x237

/* Cache control  */

#define NOT_FLUSH_TAG                 0x00000070      /* flush disable */
#define FLUSH_TAG                     0x00000071      /* flush enable  */

#endif		/* __CACHE_MEMORY_CONTROLLER__ */
