//=================================================================================
// Topro PC camera definition
//=================================================================================
//#define TP6830_QVGA
//#define TP6830_QCIF



//#define TP6830_MT9M001

//#define  DBG_4_MT9M001	1


//#define TP6830_MT9V011
//#define TP6830_MT9V111

//#define TP6830_SHARPCCD
//#define TP6830_SONYCCD


#define TP6830_TICCD

#define  CHG_4_SERCOMM		1    // Change for Sercomm project

#define  CHG_4_MT9M001		0   

//#define ISSC
//#define GCC333_UCLIB    //for CellVision
  #define CUSTOM_QTABLE		          

#define MONITOR_JJ		0

//;[#ifndef TP_DEF_H]
//;[#define TP_DEF_H]


#define UCHAR unsigned char

#ifndef TP_DEF_H
#define TP_DEF_H

  typedef unsigned char BYTE;

  struct topro_sregs{  // 'as' typedef
    unsigned char index;
    unsigned char value;
  };

#endif

//; extern
//; extern
extern int MaxExposure;

//; const
//; const
#define STOP        0
#define WAKE_UP     3

// Topro read & write command
#define READ_REGISTER   0x0D
#define WRITE_REGISTER  0x0E
#define READ_REGISTER_TYPE   		0xC0
#define WRITE_REGISTER_TYPE  		0x40
//-------------------------------------------------------------------
#define MCLK_CFG			0x01
#define MCLK_SEL			0x02
#define CLK_CFG				0x03
#define PCLK_CFG			0x04

#define PHASE_CTL1			0x05
#define PHASE_CTL2			0x06
#define PHASE_CTL3			0x07
#define PHASE_CTL4			0x08
#define PHASE_CTL5			0x09
#define PHASE_CTL6			0x0A

#define EEPROM_CTL			0x0B
#define DEV_ADR				0x0C
#define EEPROM_TX_DATA		0x0D
#define EEPROM_RX_DATA		0x0E
#define EEPROM_ADR			0x0F


#define SIF_TYPE                0x10
#define SIF_CONTROL				0x11
#define SIF_ADDR_S1				0x12
#define SIF_ADDR_S2				0x13

#define SIF_TX_DATA1			0x14
#define SIF_TX_DATA2			0x15
#define SIF_TX_DATA3			0x16
#define SIF_RX_DATA1			0x17
#define SIF_RX_DATA2			0x18


#define GPIO_PU		               0x19
#define GPIO_PD		               0x1A
#define GPIO_IO		               0x1B
#define GPIO_SUSPEND_PU				0x1C
#define GPIO_SUSPEND_PD				0x1D
#define GPIO_DATA					0x1E

#define FUNCTION_SEL			0x1F

#define ENDP_1_CTL              0x21


#define ENDP1_MAX_PACKSIZE    	0x22
#define BULKIN_INI_ADR			0x23
#define AUDIO_ISO_EN			0x25
#define SYENGEN_MODE			0x28
#define HSYNC_BLANK_NUM_L		0x29
#define HSYNC_BLANK_NUM_H		0x2A
#define HSYNC_VALID_NUM_L		0x2B
#define HSYNC_VALID_NUM_H		0x2C
#define LINE_NUM_L				0x2D
#define LINE_NUM_H				0x2E
#define ISP_MODE			0x2F
#define RD_ADR_START			0xAE
#define VIDEO_TIMING			0x30
#define VIDEO_PTN				0x31
#define CFA_HBLANK_NUM_L			0x32
#define CFA_HBLANK_NUM_H			0x33

#define CFA_FAKE_LNUM			0x34
#define CFA_PIXEL_START_L			0x35
#define CFA_PIXEL_START_H			0x36
#define CFA_LINE_START_L			0x37
#define CFA_LINE_START_H			0x38
#define CFA_FRAME_WIDTH_L			0x39
#define CFA_FRAME_WIDTH_H			0x3A
#define CFA_FRAME_HEIGHT_L			0x3B
#define CFA_FRAME_HEIGHT_H			0x3C
#define CFA_TEST_PT_L			0x3D
#define CFA_TEST_PT_H			0x3E
#define TEST_CFAIN_MODE			0x3F
#define BC_BADPXL_CTL			0x40
#define BC_HIGH_THRLD_L			0x41
#define BC_HIGH_THRLD_H			0x42
#define BC_LOW_THRLD_L			0x43
#define BC_LOW_THRLD_H			0x44
#define BC_MEAN_THRLD_L			0x45
#define BC_MEAN_THRLD_H			0x46
#define BC_DARK_CLAMP			0x47

#define GAGEN_CTL				0x52
#define GAMMA_ADDR_L			0x53
#define GAMMA_ADDR_H			0x54

#define GAMMA_R                 0x55
#define GAMMA_G                 0x56
#define GAMMA_B                 0x57
#define QTABLE_ENTRY			0x58


#define AWB_RGAIN			0x48
#define AWB_GGAIN			0x49
#define AWB_BGAIN			0x4A

#define		AWB_GAIN_DEF	0x40  //; (64)JJ-


#define AWB_ROFFSET			0x4B
#define AWB_GOFFSET			0x4c
#define AWB_BOFFSET			0x4D
#define GDIFF_THRLD			0x5F

/*
#define Y_GAIN_RL               0x60   - 3x3 Matrix
#define Y_GAIN_RH               0x61
#define Y_GAIN_GL               0x62
#define Y_GAIN_GH               0x63
#define Y_GAIN_BL               0x64
#define Y_GAIN_BH               0x65

#define U_GAIN_RL               0x66
#define U_GAIN_RH               0x67
#define U_GAIN_GL               0x68
#define U_GAIN_GH               0x69
#define U_GAIN_BL               0x6A
#define U_GAIN_BH               0x6B

#define V_GAIN_RL               0x6C
#define V_GAIN_RH               0x6D
#define V_GAIN_GL               0x6E
#define V_GAIN_GH               0x6F
#define V_GAIN_BL               0x70
#define V_GAIN_BH               0x71

 */
#define HUE_COS				0x73
#define HUE_SIN				0x74
#define SATURATION			0x75
#define BRIGHTNESS			0x76
#define CONTRAST			0x77
#define DENOISE_CFG			0x78
#define BLUR_CFG			0x79
#define SHARP_EN			0x7A
#define SHARP_CFG			0x7B
#define SHARP_CORING_TH			0x7C
#define SCALE_CFG			0x80
#define CCIR_MODE     0x81

#define JPEG_FUNC                 0x98  //[0~3] QUALITY
#define AUTOQ_FUNC               0x99
#define B_FULL1_TH				0x9A
#define B_FULL2_TH				0x9B
#define FIFO_CFG				0x9C
#define QTABLE_CFG				0x9D
#define BUFFER_STATUS           0x9E
#define MOTION_YTH_L          0xAC
#define MOTION_YTH_H          0xAD
#define TIME_CTL              0xC0
#define TIME_FORMAT           0xC1
#define YEAR_H                0xC2
#define YEAR_L                0xC3
#define MONTH                 0xC4
#define DAY                   0xC5
#define HOUR                  0xC6
#define MIN                   0xC7
#define SEC                   0xC8
#define TIME_LOCX             0xC9
#define TIME_LOCY             0xCA
#define RTD_Y                 0xCB
#define RTD_U                 0xCC
#define RTD_V                 0xCD
#define SET_YUV               0xCE

#define END_REG                 0xFF

#define GAMMA_TABLE_LENGTH      1024
#define GAMMA_TABLE_LENGTH_6830      512
#define GAIN_TABLE_LENGTH       18
#define TP6830_HEADER_SIZE       12

#define maximum(x, y)         ((x > y) ? x : y)
#define minimum(x, y)         ((x > y) ? y : x)

#define USB_CTRL_SET_TIMEOUT		200

/*
void topro_cam_init(struct usb_device *udev);

unsigned char topro_read_reg(struct usb_device *udev, unsigned char index);
int topro_write_reg(struct usb_device *udev, unsigned char index, unsigned char data);
int topro_write_i2c(struct usb_device *udev, unsigned char dev, unsigned char reg, unsigned char data_h, unsigned char data_l);
int topro_prog_regs(struct usb_device *udev, struct topro_sregs *ptpara);

int topro_setting_iso(struct usb_device *udev, unsigned char enset);

int topro_bulk_out(struct usb_device *udev, unsigned char bulkctrl, unsigned char *pbulkdata, int bulklen);


int topro_prog_gamma(struct usb_device *udev, unsigned char bulkctrl);
int topro_prog_gain(struct usb_device *udev, unsigned char *pgain);

void _topro_set_hue_saturation(struct usb_device *udev, long hue, long saturation);
void topro_autowhitebalance(struct usb_device *udev);
void topro_autoexposure(struct usb_device *udev);
void topro_SetAutoQuality(struct usb_device *udev);
typedef struct _Rtc_Data{
  int year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char minute;
  unsigned char second;
  }Tp_Rtc_Data,*PTp_Rtc_Data;
#ifdef CUSTOM_QTABLE
//void UpdateQTable(struct usb_device *udev, UCHAR value);
#endif
*/


//;function-prototype
//;function-prototype
void topro_freq_control(void);

void topro_TI_MaxExpo(void);

int CheckRange_Exposure(int dMin, int dMax, int dNew);												// JJ-

void topro_fps_control(struct usb_device *udev, int expo_was, int expo_is);		// JJ-



unsigned char CheckGainData(unsigned char Calc, unsigned char ModifyRecord);	// JJ-


BYTE Get_TableItem(int expo, int nItem);																			// JJ-


//; [#endif]


