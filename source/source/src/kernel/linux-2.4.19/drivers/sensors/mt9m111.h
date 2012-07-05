#ifndef __MT9M111_H__
#define __MT9M111_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-proc.h>
#include <linux/init.h>
#include <video/plsensor.h>

#define	MT9M111_CHIP_VER		0x143A
#define	MT9M111_SYSCTL_READ		0x1000
#define	MT9M111_SYSCTL_WRITE	0x1001

//	Shadow register
#define SHDREG_CTXT_CTL			0xC8
#define SHDREG_PG_MAP			0xF0
#define SHDREG_BYTE_WISE		0xF1

//	Sensor Core registers; page #0...
//	write 0 to register 0xF0 to switch to page #0

#define PG0REG_CHIP_VER_00		0x00
#define PG0REG_ROW_START		0x01
#define PG0REG_COL_START		0x02
#define PG0REG_WIN_HEIGHT		0x03
#define PG0REG_WIN_WIDTH		0x04
#define PG0REG_H_BLANK_B		0x05
#define PG0REG_V_BLANK_B		0x06
#define PG0REG_H_BLANK_A		0x07
#define PG0REG_V_BLANK_A		0x08
#define PG0REG_SHUTTER_WIDTH	0x09
#define PG0REG_ROW_SPEED		0x0A
#define PG0REG_EX_DELAY			0x0B
#define PG0REG_SHUTTER_DELAY	0x0C
#define PG0REG_RESET			0x0D
#define PG0REG_RD_MODE_B		0x20
#define PG0REG_RD_MODE_A		0x21
#define PG0REG_FLASH_CTL		0x23
#define PG0REG_GREEN1_GAIN		0x2B
#define PG0REG_BLUE_GAIN		0x2C
#define PG0REG_RED_GAIN			0x2D
#define PG0REG_GREEN2_GAIN		0x2E
#define PG0REG_GLOBAL_GAIN		0x2F
#define PG0REG_CHIP_VER_FF		0xFF

// colourpipe registers; page #1
//	write 1 to register 0xF0 to switch to page #1

#define PG1REG_APERTURE_CORR	0x05
#define PG1REG_OP_MODE_CTL		0x06
#define PG1REG_O_FMT_CTL		0x08
#define PG1REG_SATURATION_CTL	0x25
#define PG1REG_LUMA_OFFSET		0x34
#define PG1REG_LUMA_CLIP		0x35
#define PG1REG_O_FMT_CTL2_A		0x3A
#define PG1REG_TEST_CTL			0x48
#define PG1REG_DEFECT_CORR_A	0x4C
#define PG1REG_DEFECT_CORR_B	0x4D
#define PG1REG_LINE_CNT			0x99
#define PG1REG_FRAME_CNT		0x9A
#define PG1REG_O_FMT_CTL2_B		0x9B
#define PG1REG_REDU_H_PAN_B		0x9F
#define PG1REG_REDU_H_ZOOM_B	0xA0
#define PG1REG_REDU_H_O_SIZE_B	0xA1
#define PG1REG_REDU_V_PAN_B		0xA2
#define PG1REG_REDU_V_ZOOM_B	0xA3
#define PG1REG_REDU_V_O_SIZE_B	0xA4
#define PG1REG_REDU_H_PAN_A		0xA5
#define PG1REG_REDU_H_ZOOM_A	0xA6
#define PG1REG_REDU_H_O_SIZE_A	0xA7
#define PG1REG_REDU_V_PAN_A		0xA8
#define PG1REG_REDU_V_ZOOM_A	0xA9
#define PG1REG_REDU_V_O_SIZE_A	0xAA
#define PG1REG_REDU_CURR_H_ZOOM	0xAB
#define PG1REG_REDU_CURR_V_ZOOM	0xAC
#define PG1REG_REDU_ZOOM_SS		0xAE
#define PG1REG_REDU_ZOOM_CTL	0xAF
#define PG1REG_GBL_CLK_CTL		0xB3
#define PG1REG_EFFECTS_MODE		0xE2
#define PG1REG_EFFECTS_SEPIA	0xE3

//	camera control registers; page #2
//	write 2 to register 0xF0 to switch to page #2

#define PG2REG_AE_WIN_H_BOUND	0x26
#define PG2REG_AE_WIN_V_BOUND	0x27
#define PG2REG_AE_CWIN_H_BOUND	0x2B
#define PG2REG_AE_CWIN_V_BOUND	0x2C
#define PG2REG_AWB_WIN_BOUND	0x2D
#define PG2REG_AE_TG_PREC_CTL	0x2E
#define PG2REG_AE_SPD_SEN_CTL_A	0x2F
#define PG2REG_SHUT_WLIM_AE		0x37
#define PG2REG_FLICKER_CTL		0x5B
#define PG2REG_AE_DIG_GAIN_MON	0x62
#define PG2REG_AE_DIG_GAIN_LIM	0x67
#define PG2REG_AE_SPD_SEN_CTL_B	0x9C


typedef struct _tagMT9M111Data 
{
	int 				nSysCtlID;
	struct semaphore	UpdateLock;
	char 				cValid;			// !=0 if following fields are valid
	unsigned long		uLastUpdated;	// In jiffies 
	unsigned short		wReg;
	unsigned char 		wRead, wWrite;// Register values
	devfs_handle_t		DevFS;
	struct i2c_client	*pClient;
	PSENSOR				pSensor;
} 	MT9M111DATA;

typedef MT9M111DATA *PMT9M111DATA;

#endif
