#ifndef __OV9653_H__
#define __OV9653_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-proc.h>
#include <linux/init.h>
#include <video/plsensor.h>

#define	OV9653_SYSCTL_READ		0x1000
#define	OV9653_SYSCTL_WRITE		0x1001

#define OV9653REG_GAIN			0x00
#define OV9653REG_BLUE			0x01
#define OV9653REG_RED			0x02
#define OV9653REG_VREF			0x03
#define OV9653REG_COM1			0x04
#define OV9653REG_BAVE			0x05
#define OV9653REG_GEAVE			0x06
//	reserved					0x07
#define OV9653REG_RAVE			0x08
#define OV9653REG_COM2			0x09
#define OV9653REG_PID			0x0A
#define OV9653REG_VER			0x0B
#define OV9653REG_COM3			0x0C
#define OV9653REG_COM4			0x0D
#define OV9653REG_COM5			0x0E
#define OV9653REG_COM6			0x0F
#define OV9653REG_AECH			0x10
#define OV9653REG_CLKRC			0x11
#define OV9653REG_COM7			0x12
#define OV9653REG_COM8			0x13
#define OV9653REG_COM9			0x14
#define OV9653REG_COM10			0x15
//	reserved					0x16
#define OV9653REG_HSTART		0x17
#define OV9653REG_HSTOP			0x18
#define OV9653REG_VSTART		0x19
#define OV9653REG_VSTOP			0x1A
#define OV9653REG_PSHFT			0x1B
#define OV9653REG_MIDH			0x1C
#define OV9653REG_MIDL			0x1D
#define OV9653REG_MVFP			0x1E
#define OV9653REG_LAEC			0x1F
#define OV9653REG_BOS			0x20
#define OV9653REG_GBOS			0x21
#define OV9653REG_GROS			0x22
#define OV9653REG_ROS			0x23
#define OV9653REG_AEW			0x24
#define OV9653REG_AEB			0x25
#define OV9653REG_VPT			0x26
#define OV9653REG_BBIAS			0x27
#define OV9653REG_GbBIAS		0x28
#define	OV9653REG_GrCOM			0x29
#define OV9653REG_EXHCH			0x2A
#define OV9653REG_EXHCL			0x2B
#define OV9653REG_RBIAS			0x2C
#define OV9653REG_ADVFL			0x2D
#define OV9653REG_ADVFH			0x2E
#define OV9653REG_YAVE			0x2F
#define OV9653REG_HSYST			0x30
#define OV9653REG_HSYEN			0x31
#define OV9653REG_HREF			0x32
#define OV9653REG_CHLF			0x33
#define OV9653REG_ARBLM			0x34
//	reserved					0x35
//	reserved					0x36
#define OV9653REG_ADC			0x37
#define OV9653REG_ACOM			0x38
#define OV9653REG_OFON			0x39
#define OV9653REG_TSLB			0x3A
#define OV9653REG_COM11			0x3B
#define OV9653REG_COM12			0x3C
#define OV9653REG_COM13			0x3D
#define OV9653REG_COM14			0x3E
#define OV9653REG_EDGE			0x3F
#define OV9653REG_COM15			0x40
#define OV9653REG_COM16			0x41
#define OV9653REG_COM17			0x42
//	reserved					0x43~0x4E
#define OV9653REG_MTX1			0x4F
#define OV9653REG_MTX2			0x50
#define OV9653REG_MTX3			0x51
#define OV9653REG_MTX4			0x52
#define OV9653REG_MTX5			0x53
#define OV9653REG_MTX6			0x54
#define OV9653REG_MTX7			0x55
#define OV9653REG_MTX8			0x56
#define OV9653REG_MTX9			0x57
#define OV9653REG_MTXS			0x58
//	reserved					0x59~0x61
#define OV9653REG_LCC1			0x62
#define OV9653REG_LCC2			0x63
#define OV9653REG_LCC3			0x64
#define OV9653REG_LCC4			0x65
#define OV9653REG_LCC5			0x66
#define OV9653REG_MANU			0x67
#define OV9653REG_MANV			0x68
#define OV9653REG_HV			0x69
#define OV9653REG_MBD			0x6A
#define OV9653REG_DBLV			0x6B
//#define OV9653REG_GSP			0x6C~0x7B
//#define OV9653REG_GST			0x7C~0x8A
#define OV9653REG_COM21			0x8B
#define OV9653REG_COM22			0x8C
#define OV9653REG_COM23			0x8D
#define OV9653REG_COM24			0x8E
#define OV9653REG_DBLC1			0x8F
#define OV9653REG_DBLC_B		0x90
#define OV9653REG_DBLC_R		0x91
#define OV9653REG_DM_LML		0x92
#define OV9653REG_DM_LMH		0x93
#define OV9653REG_LCCFB			0x9D
#define OV9653REG_LCCFR			0x9E
#define OV9653REG_DBLC_Gb		0x9F
#define OV9653REG_DBLC_Gr		0xA0
#define OV9653REG_AECHM			0xA1
#define OV9653REG_COM25			0xA4
#define OV9653REG_COM26			0xA5
#define OV9653REG_G_GAIN		0xA6
#define OV9653REG_VGA_ST		0xA7

#define	OV9653_I2C_IOADDR		0x30

#define OV9653_PID				0x96
#define OV9653_VER				0x52

#define COM7_SXVGA				0x00
#define COM7_VGA				0x40
#define COM7_CIF				0x20
#define COM7_QVGA				0x10
#define COM7_QCIF				0x08
#define COM7_RGB				0x04
#define COM7_RAWRGB				0x05


typedef struct _tagOV9653Data 
{
	int 				nSysCtlID;
	struct semaphore	UpdateLock;
	char 				cValid;			// !=0 if following fields are valid
	unsigned long		uLastUpdated;	// In jiffies 
	unsigned short		wReg;
	unsigned char 		wRead, wWrite;	// Register values
	devfs_handle_t		DevFS;
	struct i2c_client	*pClient;
	PSENSOR				pSensor;
} 	OV9653DATA;

typedef OV9653DATA *POV9653DATA;

#endif
