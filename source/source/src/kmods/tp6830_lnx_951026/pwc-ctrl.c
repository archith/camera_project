/* Driver for Philips webcam
   Functions that send various control messages to the webcam, including
   video modes.
   (C) 1999-2001 Nemosoft Unv. (webcam@smcc.demon.nl)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
   Changes
   2001/08/03  Alvarado   Added methods for changing white balance and
                          red/green gains
 */

/* Control functions for the cam; brightness, contrast, video mode, etc. */

/* =================================================================================
	03/03/31 加入TOPRO's camera control command
   ================================================================================= */


#ifdef __KERNEL__
#include <asm/uaccess.h>
#endif
#include <asm/errno.h>
#include <linux/usb.h>

#include "pwc.h"
#include "pwc-ioctl.h"
#include "pwc-uncompress.h"
#include "osd1.h"
// topro header
#include "tp_def.h"
#include "pwc-ctrl-TI.h"

#include "tp_gamma_r.h"
#include "tp_gamma_g.h"
#include "tp_gamma_b.h"
#include "tp_gamma_y.h"
#include "tp_gamma2_r.h"  //950903
#include "tp_gamma2_g.h"  //950903
#include "tp_gamma2_b.h"  //950903

#include "tp_param.h"
#include "lnx.h"


#include "tp_iri_thread.h"

//#include <signal.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <fcntl.h>

#include <linux/unistd.h>
/* Request types: video */

#define SET_LUM_CTL			0x01
#define GET_LUM_CTL			0x02
#define SET_CHROM_CTL			0x03
#define GET_CHROM_CTL			0x04
#define SET_STATUS_CTL			0x05
#define GET_STATUS_CTL			0x06
#define SET_EP_STREAM_CTL		0x07
#define GET_EP_STREAM_CTL		0x08

/* Selectors for the Luminance controls [G/S]ET_LUM_CTL */
#define AGC_MODE_FORMATTER			0x2000
#define PRESET_AGC_FORMATTER			0x2100
#define SHUTTER_MODE_FORMATTER			0x2200
#define PRESET_SHUTTER_FORMATTER		0x2300
#define PRESET_CONTOUR_FORMATTER		0x2400
#define AUTO_CONTOUR_FORMATTER			0x2500
#define BACK_LIGHT_COMPENSATION_FORMATTER	0x2600
#define CONTRAST_FORMATTER			0x2700
#define DYNAMIC_NOISE_CONTROL_FORMATTER		0x2800
#define FLICKERLESS_MODE_FORMATTER		0x2900
#define AE_CONTROL_SPEED			0x2A00
#define BRIGHTNESS_FORMATTER			0x2B00
#define GAMMA_FORMATTER				0x2C00

/* Selectors for the Chrominance controls [G/S]ET_CHROM_CTL */
#define WB_MODE_FORMATTER			0x1000
#define AWB_CONTROL_SPEED_FORMATTER		0x1100
#define AWB_CONTROL_DELAY_FORMATTER		0x1200
#define PRESET_MANUAL_RED_GAIN_FORMATTER	0x1300
#define PRESET_MANUAL_BLUE_GAIN_FORMATTER	0x1400
#define COLOUR_MODE_FORMATTER			0x1500
#define SATURATION_MODE_FORMATTER1		0x1600
#define SATURATION_MODE_FORMATTER2		0x1700

/* Selectors for the Status controls [G/S]ET_STATUS_CTL */
#define SAVE_USER_DEFAULTS_FORMATTER		0x0200
#define RESTORE_USER_DEFAULTS_FORMATTER		0x0300
#define RESTORE_FACTORY_DEFAULTS_FORMATTER	0x0400
#define READ_AGC_FORMATTER			0x0500
#define READ_SHUTTER_FORMATTER			0x0600
#define READ_RED_GAIN_FORMATTER			0x0700
#define READ_BLUE_GAIN_FORMATTER		0x0800
#define READ_RAW_Y_MEAN_FORMATTER		0x3100
#define SET_POWER_SAVE_MODE_FORMATTER		0x3200
#define MIRROR_IMAGE_FORMATTER			0x3300
#define LED_FORMATTER				0x3400
#define VGA             1
#define QVGA            2


#define KILL_9M001_EXPOSURE	14  //15, 16 // Exposure 初始值, 不要太高, or 會有亮處偏紅, 拉不回來狀況!



// debug message
//#define _EXP_DBMSG_

int AEThreshold = 125;
int Extra_RateAE = 0;
int IrisTest = 0;

// solve the exposure flick
#define _FLICKER_TOLERANCE_


#ifdef _FLICKER_TOLERANCE_
int flicker_tolerance = 0;
int flicker_last_direct = 0;
int flicker_current_direct = 0;
int flicker_allow_tolerance = 18;

#endif



/* Formatters for the Video Endpoint controls [G/S]ET_EP_STREAM_CTL */
#define VIDEO_OUTPUT_CONTROL_FORMATTER		0x0100

unsigned char GlobalGain=0x50;
int RFactor=50, GFactor=50, BFactor=50;
int topro_brightness=58;  //0~100
int topro_contrast=61;    //0~100
int topro_gamma=10;       //0~30

#ifdef TP6830_TICCD
  int topro_hue= 50-0;					// To be defined, 20061025,  JJ-
  int topro_saturation= 50+0;
#else
  int topro_hue=50;							//0~100
  int topro_saturation=50;      //0~100
#endif


#ifdef TP6830_TICCD
#endif


int ShowOSD,ShowRTC;

int pwc_hframec= 0;
int ImageQuality;

int AutoExposure=0,AutoWhiteBalance=0,AutoImageQuality=0, autoqualitychecked=0;  //950209

  int MaxExposure;
  int PreExposure = 0;

#ifdef TP6830_MT9M001 // micron (Kill AE)  // Exposure 初始值, 不要太高, or 會有亮處偏紅, 拉不回來狀況!
  int Frequency=60;  //by user selected (indoor ~ 50 or 60 , outdoor ~ 0)
  int CurrentExposure= KILL_9M001_EXPOSURE;

#elif defined TP6830_TICCD
  int Frequency=60;
  int CurrentExposure= TI_MAX_EXPO_60 /2; //(7+4+12); //(4+5); // "or TI_MAX_EXPO_60 /3"
	//int IrisPrevExposure= TI_MAX_EXPO_60 /2;
    int IrisPrevIris= TI_MAX_IRIS;
    int sysIrisCount= 0;
  long AccYSmall= 256, AccYLarge= 0;

#else
  int Frequency=60;
  int CurrentExposure= 20;

#endif

int FrameSkipCount=0;

int AWB_FACTOR[3] = {0,0,0};
unsigned char AWB_GainR= 0x40;  // (64)
unsigned char AWB_GainB= 0x40;  // (64)

int ExposureTime;
UCHAR BlockAvgY[16], Motion_Ythrld=0x30;
int HwMotion_Ythrld = 0x1ff;

unsigned long MotionStatus;

unsigned char topro_read_reg(struct usb_device *udev, unsigned char index);
int topro_write_reg(struct usb_device *udev, unsigned char index, unsigned char data);
int topro_write_i2c(struct usb_device *udev, unsigned char dev, unsigned char reg, unsigned char data_h, unsigned char data_l);
int topro_prog_regs(struct usb_device *udev, struct topro_sregs *ptpara);

int topro_setting_iso(struct usb_device *udev, unsigned char enset);

int topro_bulk_out(struct usb_device *udev, unsigned char bulkctrl, unsigned char *pbulkdata, int bulklen);


int topro_prog_gamma(struct pwc_device *pdev, unsigned char bulkctrl);  //950903
int topro_prog_gamma1(struct pwc_device *pdev, unsigned char bulkctrl);  //971226
int topro_prog_gain(struct usb_device *udev, unsigned char *pgain);

//; void _topro_set_hue_saturation(struct usb_device *udev, long hue, long saturation);

//; void _topro_autowhitebalance(struct usb_device *udev);
void topro_autoexposure(struct pwc_device *pdev);		//950903

//; void _topro_SetAutoQuality(struct usb_device *udev);

typedef struct _Rtc_Data{
  int year;
  unsigned char month;
  unsigned char day;




  unsigned char hour;
  unsigned char minute;
  unsigned char second;
  }Tp_Rtc_Data,*PTp_Rtc_Data;
#ifdef CUSTOM_QTABLE
void UpdateQTable(struct usb_device *udev, UCHAR value);
#endif


int GetBulkInAvgY(struct usb_device *udev);
void GetBulkAvgRGB(struct usb_device *udev, unsigned char* AvgB, unsigned char* AvgG, unsigned char* AvgR);

// _void _topro_setexposure(struct usb_device *udev, int Exposure);

void topro_setquality(struct usb_device *udev, int quality);
unsigned long SwDetectMotion(struct pwc_device *pdev);
void topro_set_edge_enhance(struct usb_device *udev);
int topro_set_osd(struct usb_device *udev, int index);

// _void topro_config_rtc(struct usb_device *udev);
// _void update_rtc_time(struct usb_device *udev, Tp_Rtc_Data *pRtc_Data);
void topro_config_rtc(struct usb_device *udev);
void update_rtc_time(struct usb_device *udev, Tp_Rtc_Data *pRtc_Data);

void CalculateCBG(unsigned char *pData, int Contrast, int Brightness, int Gamma);
void topro_set_parameter(struct usb_device *udev);
extern int pwc_isoc_init(struct pwc_device *pdev);
extern void pwc_isoc_cleanup(struct pwc_device *pdev);

int BitmapWidth, BitmapHeight;
int FrameRate;
Tp_Rtc_Data Rtc_Data;
  
#define OSD_FILE_PATH "/home/shhsu/tp6830/osd_font1.bmp"
extern unsigned char jpg_hd[];

static char *size2name[PSZ_MAX] =
{
	"subQCIF",
	"QSIF",
	"QCIF",
	"SIF",
	"CIF",
	"VGA",
};  

/********/

/* Entries for the Nala (645/646) camera; the Nala doesn't have compression 
   preferences, so you either get compressed or non-compressed streams.
   
   An alternate value of 0 means this mode is not available at all.
 */

struct Nala_table_entry {
	char alternate;			/* USB alternate setting */
	int compressed;			/* Compressed yes/no */

	unsigned char mode[3];		/* precomputed mode table */
};

static struct Nala_table_entry Nala_table[PSZ_MAX][8] =

{
#include "pwc_nala.h"
};

/* This tables contains entries for the 675/680/690 (Timon) camera, with

   4 different qualities (no compression, low, medium, high).

   It lists the bandwidth requirements for said mode by its alternate interface 
   number. An alternate of 0 means that the mode is unavailable.
   
   There are 6 * 4 * 4 entries: 

     6 different resolutions subqcif, qsif, qcif, sif, cif, vga
     6 framerates: 5, 10, 15, 20, 25, 30
     4 compression modi: none, low, medium, high
     

   When an uncompressed mode is not available, the next available compressed mode 

   will be choosen (unless the decompressor is absent). Sometimes there are only
   1 or 2 compressed modes available; in that case entries are duplicated.

*/
struct Timon_table_entry 
{
	char alternate;			/* USB alternate interface */
	unsigned short packetsize;	/* Normal packet size */
	unsigned short bandlength;	/* Bandlength when decompressing */
	unsigned char mode[13];		/* precomputed mode settings for cam */
};

static struct Timon_table_entry Timon_table[PSZ_MAX][6][4] = 
{

#include "pwc_timon.h"

};

/* Entries for the Kiara (730/740/750) camera */

struct Kiara_table_entry
{
	char alternate;			/* USB alternate interface */

	unsigned short packetsize;	/* Normal packet size */
	unsigned short bandlength;	/* Bandlength when decompressing */
	unsigned char mode[12];		/* precomputed mode settings for cam */
};

static struct Kiara_table_entry Kiara_table[PSZ_MAX][6][4] =
{
#include "pwc_kiara.h"
};


/****************************************************************************/

void topro_freq_control(void)
{
#ifdef TP6830_TICCD 
	Frequency= 60;
#endif
}

int CheckRange_Exposure(int dMin, int dMax, int dNew)
{
	//;; int range_expo= dNew;
  dNew = max(dMin,(int)dNew);									// 'Large equal to'= 0 JJ-
  dNew = min(dMax,(int)dNew);									// Since [1~8] or [1~14], So 'MaxExposure'= 8 or 14 JJ-
	/*
  printk("TP6830:: CurrentExposure %d %d\n",dNew, dMax);
	*/
	return dNew;
}


void topro_fps_control(struct usb_device *udev, int expo_was, int expo_is)
{
#ifdef TP6830_TICCD 
		unsigned char fps_was, fps_is;
//; fps control:

		expo_is= CheckRange_Exposure(1, MaxExposure, expo_is);

		fps_was= Get_FrameRate(expo_was);
		fps_is= Get_FrameRate(expo_is);

		if (fps_was!=fps_is)
		{
			#ifdef _EXP_DBMSG_
			printk("1.-----:fps=%d:-----\n",fps_is);
			#endif
			switch(fps_is)
			{
			case 30:
				topro_write_reg(udev, MCLK_SEL, 0x11);
				#ifdef _EXP_DBMSG_
				printk("2.-----:fps=%d:-----\n",fps_is);
				#endif
				break;
			case 25:
				topro_write_reg(udev, MCLK_SEL, 0x04);
				#ifdef _EXP_DBMSG_
				printk("2.-----:fps=%d:-----\n",fps_is);
				#endif
				break;
			case 20:
				topro_write_reg(udev, MCLK_SEL, 0x12);
				#ifdef _EXP_DBMSG_
				printk("2.-----:fps=%d:-----\n",fps_is);
				#endif
				break;
			case 15:
				topro_write_reg(udev, MCLK_SEL, 0x21);
				#ifdef _EXP_DBMSG_
				printk("2.-----:fps=%d:-----\n",fps_is);
				#endif
				break;
			case 13:
				topro_write_reg(udev, MCLK_SEL, 0x08);
				#ifdef _EXP_DBMSG_
				printk("2.-----:fps=%d:-----\n",fps_is);
				#endif
				break;
			case 10:
				topro_write_reg(udev, MCLK_SEL, 0x15);  // 0x22
				#ifdef _EXP_DBMSG_
				printk("2.-----:fps=%d:-----\n",fps_is);
				#endif
				break;
			}
			#ifdef _EXP_DBMSG_
			printk("3.-----:fps=%d:-----\n",fps_is);
			#endif
			topro_write_reg(udev, MCLK_CFG, 0x03);   // SREG= MCLK_CFG= 01h
		}
#endif
		CurrentExposure= expo_is;
}




#if PWC_DEBUG
void pwc_hexdump(void *p, int len)
{
	int i;
	unsigned char *s;



	char buf[100], *d;





	s = (unsigned char *)p;
	d = buf;
	*d = '\0';
	Debug("Doing hexdump @ %p, %d bytes.\n", p, len);
	for (i = 0; i < len; i++) {
		d += sprintf(d, "%02X ", *s++);

		if ((i & 0xF) == 0xF) {
			Debug("%s\n", buf);
			d = buf;
			*d = '\0';
		}
	}
	if ((i & 0xF) != 0)
		Debug("%s\n", buf);
}
#endif

static inline int send_video_command(struct usb_device *udev, int index, void *buf, int buflen)
{
	int i;   


	    i = usb_control_msg(udev,
		usb_sndctrlpipe(udev, 0),

		SET_EP_STREAM_CTL,



		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,

		VIDEO_OUTPUT_CONTROL_FORMATTER,
		index,
		buf, buflen, HZ);



	printk("send_cmd return %d/%08x\n",i,usb_sndctrlpipe(udev, 0));



	return i;
}



static inline int set_video_mode_Nala(struct pwc_device *pdev, int size, int frames)

{
	unsigned char buf[3];
	int ret, fps;
	struct Nala_table_entry *pEntry;






	int frames2frames[31] =
	{ /* closest match of framerate */
	   0,  0,  0,  0,  4,  /*  0-4  */
	   5,  5,  7,  7, 10,  /*  5-9  */
          10, 10, 12, 12, 15,  /* 10-14 */
          15, 15, 15, 20, 20,  /* 15-19 */
          20, 20, 20, 24, 24,  /* 20-24 */

          24, 24, 24, 24, 24,  /* 25-29 */

          24                   /* 30    */
	};
	int frames2table[31] =

	{ 0, 0, 0, 0, 0, /*  0-4  */

	  1, 1, 1, 2, 2, /*  5-9  */

	  3, 3, 4, 4, 4, /* 10-14 */

	  5, 5, 5, 5, 5, /* 15-19 */
	  6, 6, 6, 6, 7, /* 20-24 */
	  7, 7, 7, 7, 7, /* 25-29 */
	  7              /* 30    */

	};

	


	if (size < 0 || size > PSZ_CIF || frames < 4 || frames > 25)
		return -EINVAL;
	frames = frames2frames[frames];
	fps = frames2table[frames];
	pEntry = &Nala_table[size][fps];

	if (pEntry->alternate == 0)
		return -EINVAL;

	if (pEntry->compressed && pdev->decompressor == NULL)
		return -ENOENT; /* Not supported. */

	memcpy(buf, pEntry->mode, 3);	
	ret = send_video_command(pdev->udev, pdev->vendpoint, buf, 3);

	if (ret < 0)
		return ret;


	if (pEntry->compressed)
		pdev->decompressor->init(pdev->release, buf, pdev->decompress_data);

		
	/*Set various parameters */
	pdev->vframes = frames;
	pdev->vsize = size;
	pdev->valternate = pEntry->alternate;
	pdev->image = pwc_image_sizes[size];
	pdev->frame_size = (pdev->image.x * pdev->image.y * 3) / 2;
	if (pEntry->compressed) {
		if (pdev->release < 5) { /* 4 fold compression */

			pdev->vbandlength = 528;
			pdev->frame_size /= 4;
		}
		else {










			pdev->vbandlength = 704;
			pdev->frame_size /= 3;

		}
	}
	else
		pdev->vbandlength = 0;

	return 0;
}




static inline int set_video_mode_Timon(struct pwc_device *pdev, int size, int frames, int compression, int snapshot)

{

	unsigned char buf[13];
	struct Timon_table_entry *pChoose;
	int ret, fps;

	if (size >= PSZ_MAX || frames < 5 || frames > 30 || compression < 0 || compression > 3)
		return -EINVAL;

	if (size == PSZ_VGA && frames > 15)

		return -EINVAL;


	fps = (frames / 5) - 1;







	/* Find a supported framerate with progressively higher compression ratios

	   if the preferred ratio is not available.
	*/
	pChoose = NULL;
	if (pdev->decompressor == NULL) {
#if PWC_DEBUG	

		Debug("Trying to find uncompressed mode.\n");
#endif
		pChoose = &Timon_table[size][fps][0];
	}
	else {

		while (compression <= 3) {
			pChoose = &Timon_table[size][fps][compression];

			if (pChoose->alternate != 0)
				break;
			compression++;	
		}
	}
	if (pChoose == NULL || pChoose->alternate == 0)
		return -ENOENT; /* Not supported. */


	memcpy(buf, pChoose->mode, 13);

	if (snapshot)
		buf[0] |= 0x80;
	if ( pdev->type != 800 ) {
	ret = send_video_command(pdev->udev, pdev->vendpoint, buf, 13);
	if (ret < 0)

		return ret;

	}

	if (pChoose->bandlength > 0) {

		printk("decompressor init\n");
		pdev->decompressor->init(pdev->release, buf, pdev->decompress_data);
	}
	/* Set various parameters */

	pdev->vframes = frames;

	pdev->vsize = size;
	pdev->vsnapshot = snapshot;
	if ( pdev->type != 800 ) pdev->valternate = pChoose->alternate;



	else pdev->valternate = 6;

	pdev->image = pwc_image_sizes[size];
	pdev->vbandlength = pChoose->bandlength;





	printk("pwc_image_sizes = %d\n",size);
	printk("bandlength = %d\n",pdev->vbandlength);

	if (pChoose->bandlength > 0) 

//		if ( pdev->type == 800 ) pdev->frame_size = 40960;//40960
		if ( pdev->type == 800 ) pdev->frame_size = 163840;//950213
		else pdev->frame_size = (pChoose->bandlength * pdev->image.y) / 4;
	else



//		if ( pdev->type == 800 ) pdev->frame_size = 40960;
		if ( pdev->type == 800 ) pdev->frame_size = 163840;
		else pdev->frame_size = (pdev->image.x * pdev->image.y * 12) / 8;
	printk("frame_size = %d\n",pdev->frame_size);
	return 0;
}


static inline int set_video_mode_Kiara(struct pwc_device *pdev, int size, int frames, int compression, int snapshot)

{
	struct Kiara_table_entry *pChoose;
	int fps, ret;
	unsigned char buf[12];
	
	if (size >= PSZ_MAX || frames < 5 || frames > 30 || compression < 0 || compression > 3)
		return -EINVAL;
	if (size == PSZ_VGA && frames > 15)



		return -EINVAL;
	fps = (frames / 5) - 1;
	
	/* Find a supported framerate with progressively higher compression ratios

	   if the preferred ratio is not available.
	*/
	pChoose = NULL;
	if (pdev->decompressor == NULL) {
#if PWC_DEBUG

		Debug("Trying to find uncompressed mode.\n");
#endif		
		pChoose = &Kiara_table[size][fps][0];
	}
	else {
		while (compression <= 3) {
			pChoose = &Kiara_table[size][fps][compression];

			if (pChoose->alternate != 0)
				break;
			compression++;
		}
	}

	if (pChoose == NULL || pChoose->alternate == 0)
		return -ENOENT; /* Not supported. */





	/* usb_control_msg won't take staticly allocated arrays as argument?? */
	memcpy(buf, pChoose->mode, 12);
	if (snapshot)
		buf[0] |= 0x80;


	/* Firmware bug: video endpoint is 5, but commands are sent to endpoint 4 */

	ret = send_video_command(pdev->udev, 4 /* pdev->vendpoint */, buf, 12);
	if (ret < 0)

		return ret;


	if (pChoose->bandlength > 0)
		pdev->decompressor->init(pdev->release, buf, pdev->decompress_data);

	/* All set and go */
	pdev->vframes = frames;
	pdev->vsize = size;
	pdev->vsnapshot = snapshot;
	pdev->valternate = pChoose->alternate;
	pdev->image = pwc_image_sizes[size];
	pdev->vbandlength = pChoose->bandlength;
	if (pChoose->bandlength > 0)
		pdev->frame_size = (pChoose->bandlength * pdev->image.y) / 4;
	else 



		pdev->frame_size = (pdev->image.x * pdev->image.y * 12) / 8;
	pdev->frame_size += (pdev->frame_header_size + pdev->frame_trailer_size);

	return 0;
}



/**
   @pdev: device structure
   @width: viewport width
   @height: viewport height

   @frame: framerate, in fps
   @compression: preferred compression ratio
   @snapshot: snapshot mode or streaming
 */
int pwc_set_video_mode(struct pwc_device *pdev, int width, int height, int frames, int compression, int snapshot)

{
	int ret, size;
	
	size = pwc_decode_size(pdev, width, height);

	if (size < 0) {

		Debug("Could not find suitable size.\n");

		return -ERANGE;

	}

	ret = -EINVAL;	

	printk("viewport for pdev_type: %d \n", pdev->type);

	switch(pdev->type) {

	case 645:
	case 646:
		ret = set_video_mode_Nala(pdev, size, frames);
		break;


	case 675:
	case 680:
	case 690:
	case 800:
		ret = set_video_mode_Timon(pdev, size, frames, compression, snapshot);
		break;


	case 730:
	case 740:
	case 750:
		ret = set_video_mode_Kiara(pdev, size, frames, compression, snapshot);
		break;
	}
	if (ret < 0) {


		if (ret == -ENOENT)
			Info("Video mode %s@%d fps is only supported with the decompressor module (pwcx).\n", size2name[size], frames);

		else {
			Err("Failed to set video mode %s@%d fps; return code = %d\n", size2name[size], frames, ret);

			return ret;

		}
	}
	pdev->view.x = width;
	pdev->view.y = height;

	pwc_set_image_buffer_size(pdev);
	Trace(TRACE_SIZE, "Set viewport to %dx%d, image size is %dx%d, palette = %d.\n", width, height, pwc_image_sizes[size].x, pwc_image_sizes[size].y, pdev->vpalette);

	printk("viewport %dx%d, image %d, view %d\n", width, height, pdev->image.size, pdev->view.size);
	return 0;

}


void pwc_set_image_buffer_size(struct pwc_device *pdev)
{
	int factor, i, filler = 0;

	switch(pdev->vpalette) {
	case VIDEO_PALETTE_RGB32 | 0x80:
	case VIDEO_PALETTE_RGB32:

		factor = 16;


		filler = 0;
		break;
	case VIDEO_PALETTE_RGB24 | 0x80:

	case VIDEO_PALETTE_RGB24:


		factor = 12;

		filler = 0;
		break;

	case VIDEO_PALETTE_YUYV:
	case VIDEO_PALETTE_YUV422:

		factor = 8;


		filler = 128;
		break;

	case VIDEO_PALETTE_YUV420:
	case VIDEO_PALETTE_YUV420P:
		factor = 6;
		filler = 128;
		break;
#if PWC_DEBUG



	case VIDEO_PALETTE_RAW:

		pdev->image.size = pdev->frame_size;
		pdev->view.size = pdev->frame_size;
		return;
		break;

#endif	

	default:
		factor = 0;
		break;
	}


	/* Set sizes in bytes */
	pdev->image.size = pdev->image.x * pdev->image.y * factor / 4;

	pdev->view.size  = pdev->view.x  * pdev->view.y  * factor / 4;



	/* Align offset, or you'll get some very weird results in
	   YUV420 mode... x must be multiple of 4 (to get the Y's in 
	   place), and y even (or you'll mixup U & V). This is less of a
	   problem for YUV420P.
	 */
	pdev->offset.x = ((pdev->view.x - pdev->image.x) / 2) & 0xFFFC;



	pdev->offset.y = ((pdev->view.y - pdev->image.y) / 2) & 0xFFFE;

	
	/* Fill buffers with gray or black */
	for (i = 0; i < MAX_IMAGES; i++) {

		if (pdev->image_ptr[i] != NULL)


			memset(pdev->image_ptr[i], filler, pdev->view.size);

	}


}






















/* BRIGHTNESS */

int pwc_get_brightness(struct pwc_device *pdev)
{
	char buf;

	int ret;
	
	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
		GET_LUM_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		BRIGHTNESS_FORMATTER,

		pdev->vcinterface,

		&buf, 1, HZ / 2);
	if (ret < 0)

		return ret;

printk("pwc_get_brightness retunr %d/%x/%08x\n",ret,buf,usb_rcvctrlpipe(pdev->udev, 0));






	return buf << 9;


}


int pwc_set_brightness(struct pwc_device *pdev, int value)
{

	char buf;



	if (value < 0)
		value = 0;
	if (value > 0xffff)

		value = 0xffff;
	buf = (value >> 9) & 0x7f;

	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_LUM_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		BRIGHTNESS_FORMATTER,
		pdev->vcinterface,

		&buf, 1, HZ / 2);

}


/* CONTRAST */




int pwc_get_contrast(struct pwc_device *pdev)









{

	char buf;



	int ret;
	

	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),

		GET_LUM_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		CONTRAST_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);

	if (ret < 0)
		return ret;
		

	printk("pwc_get_contrast ret %d/%x\n",ret,buf);
	return buf << 10;

}

int pwc_set_contrast(struct pwc_device *pdev, int value)

{
	char buf;

	if (value < 0)
		value = 0;

	if (value > 0xffff)
		value = 0xffff;

	buf = (value >> 10) & 0x3f;

	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),

		SET_LUM_CTL,

		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,

		CONTRAST_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);
}

/* GAMMA */

int pwc_get_gamma(struct pwc_device *pdev)
{

	char buf;
	int ret;

	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
		GET_LUM_CTL,

		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,


		GAMMA_FORMATTER,


		pdev->vcinterface,





		&buf, 1, HZ / 2);

	if (ret < 0)
		return ret;
	return buf << 11;
}

int pwc_set_gamma(struct pwc_device *pdev, int value)

{
	char buf;

	if (value < 0)
		value = 0;
	if (value > 0xffff)
		value = 0xffff;
	buf = (value >> 11) & 0x1f;
	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_LUM_CTL,

		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		GAMMA_FORMATTER,

		pdev->vcinterface,

		&buf, 1, HZ / 2);
}




/* SATURATION */




int pwc_get_saturation(struct pwc_device *pdev)
{



	char buf;
	int ret;

	if (pdev->type < 675)
		return -1;

	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
		GET_CHROM_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		pdev->type < 730 ? SATURATION_MODE_FORMATTER2 : SATURATION_MODE_FORMATTER1,


		pdev->vcinterface,

		&buf, 1, HZ / 2);


	if (ret < 0)

		return ret;

	return 32768 + buf * 327;
}


#if 0
  //: PWC IF CONTROL
int pwc_set_saturation(struct pwc_device *pdev, int value)
{
	char buf;

	if (pdev->type < 675)
		return -EINVAL;

	if (value < 0)
		value = 0;
	if (value > 0xffff)

		value = 0xffff;
	/* saturation ranges from -100 to +100 */

	buf = (value - 32768) / 327;

	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_CHROM_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		pdev->type < 730 ? SATURATION_MODE_FORMATTER2 : SATURATION_MODE_FORMATTER1,
		pdev->vcinterface,
		&buf, 1, HZ / 2);
}
#endif

/* AGC */


static inline int pwc_set_agc(struct pwc_device *pdev, int mode, int value)



{
	char buf;
	int ret;



	if (mode)


		buf = 0x0; /* auto */

	else
		buf = 0xff; /* fixed */

	ret = usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),

		SET_LUM_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		AGC_MODE_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);
	


	if (!mode && ret >= 0) {
		if (value < 0)
			value = 0;

		if (value > 0xffff)
			value = 0xffff;



		buf = (value >> 10) & 0x3F;
		ret = usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),



			SET_LUM_CTL,
			USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,

			PRESET_AGC_FORMATTER,
			pdev->vcinterface,
			&buf, 1, HZ / 2);
	}
	if (ret < 0)
		return ret;
	return 0;
}

static inline int pwc_get_agc(struct pwc_device *pdev, int *value)
{
	unsigned char buf;

	int ret;



	

	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
		GET_LUM_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		AGC_MODE_FORMATTER,






		pdev->vcinterface,


		&buf, 1, HZ / 2);
	if (ret < 0)



		return ret;


	if (buf != 0) { /* fixed */
		ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
			GET_LUM_CTL,
			USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			PRESET_AGC_FORMATTER,
			pdev->vcinterface,
			&buf, 1, HZ / 2);
		if (ret < 0)

			return ret;



		if (buf > 0x3F)
			buf = 0x3F;
		*value = (buf << 10);		
	}
	else { /* auto */
		ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),



			GET_STATUS_CTL,
			USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			READ_AGC_FORMATTER,
			pdev->vcinterface,
			&buf, 1, HZ / 2);
		if (ret < 0)

			return ret;
		/* Gah... this value ranges from 0x00 ... 0x9F */
		if (buf > 0x9F)
			buf = 0x9F;
		*value = -(48 + buf * 409);
	}

	return 0;

}


static inline int pwc_set_shutter_speed(struct pwc_device *pdev, int mode, int value)

{


	char buf[2];
	int speed, ret;


	if (mode)

		buf[0] = 0x0;	/* auto */
	else


		buf[0] = 0xff; /* fixed */
	
	ret = usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),

		SET_LUM_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		SHUTTER_MODE_FORMATTER,

		pdev->vcinterface,
		buf, 1, HZ / 2);

	if (!mode && ret >= 0) {
		if (value < 0)
			value = 0;
		if (value > 0xffff)
			value = 0xffff;
		switch(pdev->type) {

		case 675:


		case 680:

		case 690:
			/* speed ranges from 0x0 to 0x290 (656) */

			speed = (value / 100);

			buf[1] = speed >> 8;






			buf[0] = speed & 0xff;
			break;

		case 730:
		case 740:



		case 750:
			/* speed seems to range from 0x0 to 0xff */

			buf[1] = 0;
			buf[0] = value >> 8;

			break;
		}

		ret = usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
			SET_LUM_CTL,
			USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			PRESET_SHUTTER_FORMATTER,

			pdev->vcinterface,
			&buf, 2, HZ / 2);
	}
	return ret;
}



/* POWER */


int pwc_camera_power(struct pwc_device *pdev, int power)
{


	char buf;



	if (pdev->type < 675 || (pdev->type < 730 && pdev->release < 6))
		return 0;	/* Not supported by Nala or Timon < release 6 */


	if (power)

		buf = 0x00; /* active */
	else
		buf = 0xFF; /* power save */


	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_STATUS_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		SET_POWER_SAVE_MODE_FORMATTER,



		pdev->vcinterface,
		&buf, 1, HZ / 2);
}







/* private calls */

static inline int pwc_restore_user(struct pwc_device *pdev)


{
	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),


		SET_STATUS_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		RESTORE_USER_DEFAULTS_FORMATTER,
		pdev->vcinterface,
		NULL, 0, HZ / 2);
}


static inline int pwc_save_user(struct pwc_device *pdev)
{
	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),

		SET_STATUS_CTL,

		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,


		SAVE_USER_DEFAULTS_FORMATTER,
		pdev->vcinterface,
		NULL, 0, HZ / 2);
}

static inline int pwc_restore_factory(struct pwc_device *pdev)
{


	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),

		SET_STATUS_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		RESTORE_FACTORY_DEFAULTS_FORMATTER,
		pdev->vcinterface,
		NULL, 0, HZ / 2);
}

 /* ************************************************* */
 /* Patch by Alvarado: (not in the original version   */


 /*
  * the camera recognizes modes from 0 to 4:
  *

  * 00: indoor (incandescant lighting)
  * 01: outdoor (sunlight)


  * 02: fluorescent lighting

  * 03: manual
  * 04: auto
  */ 

static inline int pwc_set_awb(struct pwc_device *pdev, int mode)
{
	char buf;
	int ret;
	
	if (mode < 0)
	    mode = 0;
	

	if (mode > 4)
	    mode = 4;


	
	buf = mode & 0x07; /* just the lowest three bits */
	
	ret = usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_CHROM_CTL,


		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,

		WB_MODE_FORMATTER,

		pdev->vcinterface,
		&buf, 1, HZ / 2);

	if (ret < 0)


		return ret;
	return 0;
}



static inline int pwc_get_awb(struct pwc_device *pdev)
{


	unsigned char buf;
	int ret;
	
	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
		GET_CHROM_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,

		WB_MODE_FORMATTER,



		pdev->vcinterface,
		&buf, 1, HZ / 2);


	if (ret < 0) 
		return ret;
	return buf;
}


static inline int pwc_set_red_gain(struct pwc_device *pdev, int value)
{


        unsigned char buf;





	if (value < 0)
		value = 0;
	if (value > 0xffff)
		value = 0xffff;

	/* only the msb are considered */
	buf = value >> 8;

	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_CHROM_CTL,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		PRESET_MANUAL_RED_GAIN_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);
}




static inline int pwc_get_red_gain(struct pwc_device *pdev)




{
	unsigned char buf;




	int ret;
	
	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),


 	        GET_STATUS_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,



	        PRESET_MANUAL_RED_GAIN_FORMATTER,

		pdev->vcinterface,
		&buf, 1, HZ / 2);



	if (ret < 0)


	    return ret;
	
	return (buf << 8);
}




static inline int pwc_set_blue_gain(struct pwc_device *pdev, int value)
{

	unsigned char buf;


	if (value < 0)
		value = 0;
	if (value > 0xffff)

		value = 0xffff;

	/* linear mapping of 0..0xffff to -0x80..0x7f */

	buf = (value >> 8);


	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),

		SET_CHROM_CTL,


		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		PRESET_MANUAL_BLUE_GAIN_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);

}


static inline int pwc_get_blue_gain(struct pwc_device *pdev)


{
	unsigned char buf;
	int ret;



	printk("pwc_get_blue_gain rcvpipe = %d\n",usb_rcvctrlpipe(pdev->udev, 0));
	
	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
   	        GET_STATUS_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,

		PRESET_MANUAL_BLUE_GAIN_FORMATTER,

		pdev->vcinterface,
		&buf, 1, HZ / 2);



	if (ret < 0)
	    return ret;
	


	printk("blue gain return %d\n",ret);
	

	return (buf << 8);
}

/* The following two functions are different, since they only read the
   internal red/blue gains, which may be different from the manual 
   gains set or read above.
 */   
static inline int pwc_read_red_gain(struct pwc_device *pdev)
{







	unsigned char buf;
	int ret;

	
	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
 	        GET_STATUS_CTL, 



		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,


	        READ_RED_GAIN_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);

	if (ret < 0)



	    return ret;

	printk("red gain return %d\n",ret);
	

	return (buf << 8);
}


static inline int pwc_read_blue_gain(struct pwc_device *pdev)
{
	unsigned char buf;
	int ret;
	

	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),


   	        GET_STATUS_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		READ_BLUE_GAIN_FORMATTER,
		pdev->vcinterface,
		&buf, 1, HZ / 2);



	if (ret < 0)
	    return ret;
	
	return (buf << 8);
}

int pwc_set_leds(struct pwc_device *pdev, int on_value, int off_value)
{

	unsigned char buf[2];


	if (pdev->type < 730)
		return 0;
	on_value /= 100;

	off_value /= 100;
	if (on_value < 0)
		on_value = 0;
	if (on_value > 0xff)
		on_value = 0xff;


	if (off_value < 0)
		off_value = 0;

	if (off_value > 0xff)
		off_value = 0xff;



	buf[0] = on_value;

	buf[1] = off_value;



	return usb_control_msg(pdev->udev, usb_sndctrlpipe(pdev->udev, 0),
		SET_STATUS_CTL,

		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		LED_FORMATTER,



		pdev->vcinterface,



		&buf, 2, HZ / 2);
}

int pwc_get_leds(struct pwc_device *pdev, int *on_value, int *off_value)
{

	unsigned char buf[2];
	int ret;

	if (pdev->type < 730) {



		*on_value = -1;
		*off_value = -1;
		return 0;


	}

	ret = usb_control_msg(pdev->udev, usb_rcvctrlpipe(pdev->udev, 0),
   	        GET_STATUS_CTL,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,





		LED_FORMATTER,
		pdev->vcinterface,
		&buf, 2, HZ / 2);

	if (ret < 0)
		return ret;


	*on_value = buf[0] * 100;

	*off_value = buf[1] * 100;



	return 0;



}

 /* End of Add-Ons                                    */

 /* ************************************************* */

void s200_read_all(struct usb_device *udev)
{
#define f0	0xf0
#define f2	0xf2
#define f3	0xf3
	int i;
	unsigned char regr, datr, sum;

#if 0
	unsigned char regTab[]= {
		0x10, 0x11, /*0x12,*/ /*0x19, 0x13, 0x1a, 0x05,*/ 0x14, 0x1b, /*0x15, 0x16, 0x17, 0x40, 0x41,*/ 0x18,
		0x1c, 0x1d, 0x1e, 0x1f, 0x20,
		/*0x21,*/ 0x22, 0x23, 0x24, 0x25, 
		0x26, 0x27, 0x28, /*0x2e, 0x2f,*/
		0x30, /*0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,*/
		/*0x3a, 0x3b, 0x3c, 0x3d, 0x3e,*/ 0x3f, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
		0xb0, 
		0xa9, 0xaa, 0xab, 0xac, 0xad, 
		0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, /*0x52, 0x53, 0x54,*/ 0x55, 0x56, 0x57, /*0x58,*/ 0x02, 0x03, 0x04, 0x06,
		0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
		0x6f, 0x70, 0x71,  0x78, 0x79, 0x7a, 0x7b, /*0x7c,*/ 
		0x7d, 0x80, 0x81, 0x82,
		0x83, 0x84, 0x85,
		0xff,
	};
	unsigned char sumTab[]= {
		0x10, 0x11, /*0x12,*/ /*0x19, 0x13, 0x1a, 0x05,*/ 0x14, 0x1b, /*0x15, 0x16, 0x17, 0x40, 0x41,*/ 0,
		0x1c, 0x1d, 0x1e, 0x1f, 0x20,
		/*0x21,*/ 0x22, 0x23, 0x24, 0x25, 
		0x26, 0x27, 0x28, /*0x2e, 0x2f,*/
		0x30, /*0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,*/
		/*0x3a, 0x3b, 0x3c, 0x3d, 0x3e,*/ 0x3f, 0x07, 0x08, 0x09, 0x0a, 0x0b,  0,
		0, 
		0xa9, 0xaa, 0xab, 0xac, 0xad, 
		0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, /*0x52, 0x53, 0x54,*/    f0,    f2,    f3, /*0x58,*/ 0x02, 0x03, 0x04, 0x06,
		0x59, 0x5a, 0x5b, 0x5c, 0x5d,    0,    0,  f0   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   , f2   ,
		f2   , f2   , f3   ,  0x78, 0x79, 0x7a,  0, /*0x7c,*/ 
		0x7d,    0,    0,    0,
		   0,    0,    0,
		0xff,
	};
#else
	unsigned char regTab[]= {
		0x01, 0x02, 0x03, 0x04,/*0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,*/0x0b, 0x0c, 0x0d, 0x0e, 0x0f,  0x10, 
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,/*0x20,*/
		0x21, 0x22,/*0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,*/0x2f, 0x30,
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c,/*0x3d, 0x3e, 0x3f,  0x40,*/
		0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,/*0x4b, 0x4c, 0x4d, 0x4e, 0x4f,  0x50,
		0x51,*/0x52,/*0x53, 0x54, 0x55, 0x54, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,  0x70,
		0x71, 0x72,*/0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,  0x80,
    0x81,/*0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,*/
	/*0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97*/0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,  0xa0,
		
		0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,  0xb0,
		0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,  0xc0,
		0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,  0xd0,
		0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,  0xe0, 
		0xe1, 0xe2,
		0xff,
	};
	unsigned char sumTab[]= {
		f0, f2, f2, f2,/*0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,*/f2, f2, f2, f2, f2,  f3, 
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f3,/*0x20,*/
		f0, f2,/*0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,*/f2, f3,
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f3,/*0x3d, 0x3e, 0x3f,  0x40,*/
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f3,/*0x4b, 0x4c, 0x4d, 0x4e, 0x4f,  0x50,
		0x51,*/f3,/*0x53, 0x54, 0x55, 0x54, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,  0x70,
		0x71, 0x72,*/f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2,  f3,
    f3,/*0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,*/
	/*0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97*/f0, f2, f2, f2, f2, f2, f2, f2,  f3,
		
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2,  f3,
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2,  f3,
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2,  f3,
		f0, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2, f2,  f3, 
		f0, f3,
		0xff,
	};
#endif

	i= 0;
	sum= 0;
	while (1)
	{
		regr= regTab[i];
		if (regr==0xff)
			break;

		datr= topro_read_reg(udev, regr);

		if (sumTab[i]){

			if (sumTab[i]==f0) //sumTab[i]==0xf0
			//;printk("%d. dump reg= %.2x, data = %.2x ", i, regr, datr);
			   printk("%02x. dump reg= %.2x, data = %.2x ", (i+1), regr, datr);
			else if (sumTab[i]==f2)
			  printk("%.2x ", datr);
			else if (sumTab[i]==f3){
			  printk("%.2x ", datr);
			  printk("\n");
			}
			else
			   //if (sumTab[i]!=f0 && sumTab[i]!=f2 && sumTab[i]!=f3)
			{

				if (regr==0x4c) datr= 0xff;
				if (regr==0x4d) datr= 0x00;
				if (regr==0x4e) datr= 0xff;
				if (regr==0x4f) datr= 0x00;

				printk("%d. dump reg= %.2x, data = %.2x (%.2x) \n", i, regr, datr, (unsigned char)(sum + datr));

				if (regr==0x11) { // SIF_CONTROL
					datr &= 0xfe; // Clear TX_ACT flg
					printk("%d. dump reg= %.2x, data = %.2x (%.2x) \n", i, regr, datr, (unsigned char)(sum + datr));
				}

				sum += datr;
			}

		}
		else //sumTab[i]==0
			printk("%d. dump reg= %.2x, data = %.2x \n", i, regr, datr);

		i++;
	}
}


void set_dac1_register(struct usb_device *udev, int arg)
{
	unsigned char ucdat, xcdat;
	int value = (int)arg;

	ucdat= value;

	xcdat= ucdat/32;
	xcdat += 1;
	ucdat= ucdat % 32;
	ucdat *= 2;
	ucdat <<= 2;
	topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat);

	//printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat >> 2);
	xcdat <<= 2;
	topro_write_spi16(udev, DAC_CONTROL_MSB, xcdat);  // 0x01 << 2
	//printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, xcdat>>2);
	//printk("\n");
}


void set_dac1_register2(struct usb_device *udev, int arg)
{
	unsigned char ucdat, xcdat;
	int value = (int)arg;

	ucdat= value;

	xcdat= ucdat/32;
	xcdat += 0;
	ucdat= ucdat % 32;
	ucdat *= 2;
	ucdat <<= 2;
	topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat);

	//printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat >> 2);
	xcdat <<= 2;
	topro_write_spi16(udev, DAC_CONTROL_MSB, xcdat);  // 0x01 << 2
	//printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, xcdat>>2);
	//printk("\n");
}

void run_auto_iris_diagnosis(struct usb_device *udev, int val)
{
	IrisTest = 1;

	/*
	int i = 0;

	printk("[Diagonsis] begin\n");
	set_current_state(TASK_INTERRUPTIBLE);
	
	for(i = 0; i < 160; i+=16)
	{
		printk("[Diagonsis] set register 0x%02x\n", i);
		set_dac1_register2(udev, i);
		mdelay(1000);
	}
	printk("[Diagonsis] end\n");
	*/

	printk("IrisTest %d value: %d\n", IrisTest, val * 12);
	set_dac1_register2(udev, val * 12);
}

int pwc_ioctl(struct pwc_device *pdev, unsigned int cmd, void *arg)

{

	down(&sem);

	switch(cmd) {

	case VIDIOCPWCRUSER:

	{


		if (pwc_restore_user(pdev))

			return -EINVAL;
		break;
	}


	case VIDIOCPWCSUSER:
	{



		if (pwc_save_user(pdev))
			return -EINVAL;
		break;
	}

	case VIDIOCPWCFACTORY:


	{


		if (pwc_restore_factory(pdev))
			return -EINVAL;
		break;



	}

	case VIDIOCPWCSCQUAL:

	{
		int qual;


		if (copy_from_user(&qual, arg, sizeof(int)))
			return -EFAULT;


//		if (qual < 0 || qual > 3)
		if (qual < 0 || qual > 15)
			return -EINVAL;
//		ret = pwc_try_video_mode(pdev, pdev->view.x, pdev->view.y, pdev->vframes, qual, pdev->vsnapshot);
//		if (ret < 0)
//			return ret;

//		pdev->vcompression = qual;
  	ImageQuality = qual;

    topro_setquality(pdev->udev, ImageQuality);

		break;

	}
	
	case VIDIOCPWCGCQUAL:

	{
//		if (copy_to_user(arg, &pdev->vcompression, sizeof(int)))
		if (copy_to_user(arg, &ImageQuality, sizeof(int)))
			return -EFAULT;

		break;

	}


	case VIDIOCPWCSAGC:
	{
		int agc;
		
		if (copy_from_user(&agc, arg, sizeof(agc)))
			return -EFAULT;
		else {
			if (pwc_set_agc(pdev, agc < 0 ? 1 : 0, agc))
				return -EINVAL;

		}
		break;
	}
	
	case VIDIOCPWCGAGC:

	{


		int agc;
		
		if (pwc_get_agc(pdev, &agc))


			return -EINVAL;

		if (copy_to_user(arg, &agc, sizeof(agc)))

			return -EFAULT;

		break;
	}


	case VIDIOCPWCSSHUTTER:
	{
		int shutter_speed, ret;


		if (copy_from_user(&shutter_speed, arg, sizeof(shutter_speed)))
			return -EFAULT;
		else {

			ret = pwc_set_shutter_speed(pdev, shutter_speed < 0 ? 1 : 0, shutter_speed);

			if (ret < 0)
				return ret;

		}
		break;
	}


 /* ************************************************* */
 /* Begin of Add-Ons for color compensation           */

//950903
    case VIDIOCPWCSAWB:
	{
		struct pwc_whitebalance wb;
		int ret;
		unsigned char bw_gain[] =
		{
			// Y Gain
			0x4d,0x00,
			0x96,0x00,
			0x1d,0x00,

			// U Gain
			0x00,0x00,
			0x00,0x00,
			0x00,0x00,

			// V Gain
			0x00,0x00,
			0x00,0x00,
			0x00,0x00
		};

		if (copy_from_user(&wb, arg, sizeof(wb)))
			return -EFAULT;

		printk("VIDIOCPWCSAWB %d\n",wb.mode);

		// PWC_WB_AUTO		  4
		//;
		// PWC_WB_INDOOR	  0
		// PWC_WB_FL_Y		  3  (PWC_WB_OUTDOOR  1)
		// PWC_WB_FL				2
		// PWC_WB_BW   	    5

		if (wb.mode != pdev->awbmode)
		{
			AWB_GainR = 0x40;  // rondall
			AWB_GainB = 0x40;  // rondall

			AWB_FACTOR[0] = 0;  // rondall
			AWB_FACTOR[2] = 0;  // rondall
			
			if (wb.mode == 5){
				pdev->awbmode = wb.mode;

#if 1
				//; Gain
				if (topro_prog_gain(pdev->udev, bw_gain))
					printk("program gain error.\n");
#endif
				//; Gain
				//if (topro_prog_gain(pdev->udev, new_gain))
				//	printk("program gain error.\n");

			}
			else if (wb.mode >= 0 && wb.mode <= 5){

				pdev->awbmode= wb.mode;
				
#if 1
				//; Gain
				if (topro_prog_gain(pdev->udev, new_gain))
					printk("program gain error.\n");
#endif
				//; Gain
				// topro_set_hue_saturation(pdev->udev, topro_hue, topro_saturation);

				#if 0
				topro_write_reg(pdev->udev, GAGEN_CTL, 0x00);
					topro_write_reg(pdev->udev, GAMMA_ADDR_L, 0x00);
					topro_write_reg(pdev->udev, GAMMA_ADDR_H, 0x00);
					topro_prog_gamma(pdev, 0x00);
					topro_prog_gamma(pdev, 0x01);
					topro_prog_gamma(pdev, 0x02);
				topro_write_reg(pdev->udev, GAGEN_CTL, 0x04);
				#endif
			}
			else
				return -EFAULT;
		}
		break;
	}

//950903
	case VIDIOCPWCGAWB:
	{
		struct pwc_whitebalance wb;

		memset(&wb, 0, sizeof(wb));
	    wb.mode = pdev->awbmode;
		if (copy_to_user(arg, &wb, sizeof(wb)))
			return -EFAULT;
		printk("VIDIOCPWCGAWB %d\n",wb.mode);
		break;
	}

    case VIDIOCPWCSLED:
	{
		int ret;

		struct pwc_leds leds;



		if (copy_from_user(&leds, arg, sizeof(leds)))
			return -EFAULT;

		ret = pwc_set_leds(pdev, leds.led_on, leds.led_off);
		if (ret<0)
		    return ret;
	    break;

	}




	case VIDIOCPWCGLED:
	{

		int led;

		struct pwc_leds leds;

		led = pwc_get_leds(pdev, &leds.led_on, &leds.led_off);
		if (led < 0)
			return -EINVAL;

		if (copy_to_user(arg, &leds, sizeof(leds)))
			return -EFAULT;
		break;
	}

	//////////////////////////////////////////////////////////////////////


	// Topro IoControl
	//////////////////////////////////////////////////////////////////////

	case VIDIOCWRITEREG:
	{
		unsigned char buff[2];


		if (copy_from_user(buff, arg, 2))
			return -EFAULT;




		topro_write_reg(pdev->udev, buff[0], buff[1]);



		printk("write register index = %.2x, data = %.2x\n", buff[0], buff[1]);


		break;
	}
	case VIDIOCREADREG:
	{
		unsigned char buff[2];


		if (copy_from_user(buff, arg, 2))
			return -EFAULT;

		if (buff[0]==0)
		{
			s200_read_all(pdev->udev);
			break;
		}

		buff[1] = topro_read_reg(pdev->udev, buff[0]);

		if (copy_to_user(arg, buff, 2))
			return -EFAULT;

		printk("read register index = %.2x, data = %.2x\n", buff[0], buff[1]);
		break;
	}
	case VIDIOCSETGAMMAR:
	{
		if (copy_from_user(tp_gamma_r, arg, GAMMA_TABLE_LENGTH))
			return -EFAULT;




		if (topro_prog_gamma(pdev, 0x00)) //950903
		 printk("program gamma R error.\n");
		else
		 printk("program gamma R OK.\n");



		break;


	}
	case VIDIOCSETGAMMAG:
	{


		if (copy_from_user(tp_gamma_g, arg, GAMMA_TABLE_LENGTH))

			return -EFAULT;

		if (topro_prog_gamma(pdev, 0x01))   //950903

		 printk("program gamma G error.\n");

		else
		 printk("program gamma G OK.\n");


		break;

	}
	case VIDIOCSETGAMMAB:

	{
		if (copy_from_user(tp_gamma_b, arg, GAMMA_TABLE_LENGTH))


			return -EFAULT;


		if (topro_prog_gamma(pdev, 0x02))   //950903
		 printk("program gamma B error.\n");
		else
		 printk("program gamma B OK.\n");


		break;
	}

  
	case VIDIOCGETEXPOSURE_EX:   // 20061124 ('244')
		{
		  copy_to_user(arg, &CurrentExposure, sizeof(CurrentExposure));
		  //printk("GetExposure %d\n",*((int*)arg));
			break;
		}

 	case VIDIOCSETEXPOSURE:  // ('216') v.s. 'VIDIOCGETEXPOSURELEVEL'
	case VIDIOCSETEXPOSURE_EX:   // 20061124 ('245')
	{
		//; AccYSmall= 256; AccYLarge= 0; Done in topro set_exposure function
		
//;	printk("--------- > SetExposure %d < ---------- \n", (int)arg);
		AutoExposure = 0;
		
		topro_fps_control(pdev->udev,CurrentExposure, (int)arg);
		printk("x.-----:SetExposure %d \n", (int)arg);	// [CurrentExposure as too]
		topro_setexposure(pdev->udev,(int)arg,pdev);					// [1 ~100 ]  JJ-
		break;
	}

//951129
	case VIDIOCGETIRIS_MAX:
	{
		int maxiris= TI_MAX_IRIS;
		  copy_to_user(arg, &maxiris, sizeof(maxiris));
		  printk("GetIris_Max %d\n",*((int*)arg));
		break;
	}
	case VIDIOCGETIRIS_EX:
	{
		  copy_to_user(arg, &IrisPrevIris, sizeof(IrisPrevIris));
		  printk("GetIris %d\n",*((int*)arg));
		break;
	}
	case VIDIOCSETIRIS_EX:
	{

		unsigned char ucdat, xcdat;
		AutoExposure = 0;
		printk("x.-----:SetIris %d \n", (int)arg);	// [CurrentExposure as too]

		ucdat= (unsigned char)arg;

		xcdat= ucdat/32;
		xcdat += 1;
		//xcdat=1;
		//xcdat=2;
		ucdat= ucdat % 32;
		ucdat *= 2;
		//ucdat=;
		//ucdat=;

		ucdat <<= 2;
		topro_write_spi16(pdev->udev, DAC_CONTROL_LSB, ucdat);

		printk(" (direct) ");
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat >> 2);

		//;xcdat= 1;
		xcdat <<= 2;
		topro_write_spi16(pdev->udev, DAC_CONTROL_MSB, xcdat);  // 0x01 << 2

		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, xcdat>>2);
		printk("\n");


		break;
	}

	case VIDIOCOPENIRIS:
	{

		int value = (int)arg;

		int start = (value / 100);
		int stop = (value % 100);

		printk("VIDIOCOPENIRIS: %d (%d,%d)<-(%d, %d)\n", value, start, stop, start - 32, stop - 32);
		set_dac1_register2(pdev->udev, start);
		udelay(10);
		set_dac1_register2(pdev->udev, stop);
		break;
	}	

	case VIDIOCCLOSEIRIS:
	{
		int value = (int)arg;

		AutoExposure = 0;

		int start = (value / 100);
		int stop = (value % 100);

		printk("VIDIOCCLOSEIRIS: %d (%d,%d)\n", value, start, stop);
		set_dac1_register(pdev->udev, start);
		udelay(100);
		set_dac1_register(pdev->udev, stop);

		break;
	}	

	case VIDIOCIRISDIAGNOSIS:
	{
		int value = (int)arg;

		run_auto_iris_diagnosis(pdev->udev, value);
		break;
	}



//950903
 	case VIDIOCGETEXPOSURELEVEL:
	{
		copy_to_user(arg, &(pdev->exposurelevel), sizeof(pdev->exposurelevel));
		printk("GETEXPOSURELEVEL %d\n",*((int*)arg));
		break;
	}

//950903
 	case VIDIOCSETEXPOSURELEVEL:
	{
	  pdev->exposurelevel = (int)arg;

		AccYSmall= 256; AccYLarge= 0; 
		
		#ifdef _FLICKER_TOLERANCE_
		flicker_tolerance = 0; 
		flicker_last_direct = 0; 
		flicker_current_direct = 0;
		flicker_allow_tolerance = 10;
		#endif
		
		printk(" - - - - - - - - - - - - - - -> SETEXPOSURELEVEL  %d\n",(int)arg);
		printk("[ExposureToDarker:0,    Normal: 10,  ExposureToLighter: 21 \n]");

		break;
	}

 	case VIDIOCGETMOTION:
	{

    copy_to_user(arg, &MotionStatus, sizeof(MotionStatus));

    printk("GetMotion %d\n",(int)arg);
		break;

	}

 	case VIDIOCSETMOTION:
	{
    Motion_Ythrld = (int)arg;
    printk("SetMotion %d\n",(int)arg);
		break;
	}

 	case VIDIOCGETFREQUENCY:
	{
    copy_to_user(arg, &Frequency, sizeof(Frequency));
    printk("Get Frequency %d\n",(int)arg);
		break;
	}

 	case VIDIOCSETFREQUENCY:
	{
    if (arg == 0)
      Frequency = 0;
    else if ((int)arg == 50)
      Frequency = 50;
    else
      Frequency = 60;

    printk("Set Frequency %d\n",(int)Frequency);
		break;
	}

//950801: Add Video frame count for AE/AWB w/o ISO_IN 
 	case VIDIOCFRAMECOUNT:
	{
		//topro_autoexposure(pdev);		//950903
		//topro_autowhitebalance(pdev->udev);
		break;
	}

//950827  
 	case VIDIOCISOCSTART:
	{
    pwc_isoc_init(pdev);
    printk("VIDIOCISOCSTART\n");
    break;
	}

//950827  
 	case VIDIOCISOCSTOP:
	{
    pwc_isoc_cleanup(pdev);
    printk("VIDIOCISOCSTOP\n");
    break;
	}
	
//  case VIDIOCSETQTABLE:
//  {
//    update_qtable(pdev);
//  }


 /* End of Add-Ons                                    */
 /* ************************************************* */

	default:

		return -ENOIOCTLCMD;



		break;



	}

	up(&sem);

	return 0;

}

#define STACKSIZE 16384

#define CSIGNAL         0x000000ff      /* signal mask to be sent at exit */
#define CLONE_VM        0x00000100      /* set if VM shared between processes */
#define CLONE_FS        0x00000200      /* set if fs info shared between processes */
#define CLONE_FILES     0x00000400      /* set if open files shared between processes */
#define CLONE_SIGHAND   0x00000800      /* set if signal handlers shared */

int run_thread = 1;

void leave_thread()
{
	down(&sem);
	run_thread = 0;
	up(&sem);

	while(run_thread != -1)
	{
		schedule_timeout(25);
	}
}

static int topro_iris_thread(void* data)
{
	struct pwc_device *pdev = (struct pwc_device *)data;
	struct usb_device *udev = pdev->udev;

	int indicator = 0, i = 0;	
	int ccode = 0;

	int PreAccY, CurAccY;	// record the previous Y and the current Y

	down(&sem);
	PreAccY = CurAccY = GetBulkInAvgY(udev);
	up(&sem);

	state_init();

	while(run_thread == 1)
	{
		set_current_state(TASK_INTERRUPTIBLE);

		if(IrisTest == 1)
		{
			schedule_timeout(35);
			continue;
		}
		
		down(&sem);

		topro_autoexposure(pdev);		
		topro_autowhitebalance(udev);

		// get the indicator
		indicator = CurrentIRISIndicator;
		ccode = 0;

		// open the iris fully
		if(indicator == -9999)
		{
			// clear other variables
			valid_mode = INVALID;
			keep = MIN_KEEP;
			dac_first_register_value = 0;
			dac_second_register_value = 4;

			// set the register to open the iris
			set_dac1_register2(udev, dac_first_register_value);
			udelay(100);
			set_dac1_register2(udev, dac_second_register_value);
			udelay(500);
		
			// get the current exposure to see the change
			CurAccY = GetBulkInAvgY(udev);

			#ifdef _IRIS_DBMSG_
			printk("(CurAccY,PreAccY)->(%d,%d) %s, %d\n", CurAccY, PreAccY, CurAccY<30?"clear vv":"keep vv", last_valid_value);
			#endif

			// the change could be not so quick, however, we still try to adjust the range of the valid voltage
			// the last_valid_value is used to record the valid voltage.
			// what will happen if the current exposure is kept low? it could be the light become dark or change the lens. 
			// if so, we try to low the valid voltage that we found.
			// the valid voltage will be useless if the lens is chagned.

			if(CurAccY < 40)
			{
				if(last_valid_value > 20)
				{
					last_valid_value -= 5;
					if(last_valid_value < 20)
						last_valid_value = 20;
				}
			}

			failed_and_tolerance_range = failed_and_tolerance_range > 0 ? failed_and_tolerance_range-- : failed_and_tolerance_range;


			// update the previous exposure
			PreAccY = CurAccY;
		}

		// close the iris step by step
		if(indicator >= 20 && CurrentExposure == 1)
		{
			// get the current exposure to see the change
			CurAccY = GetBulkInAvgY(udev);
			ccode = CheckCurrentAvgY(CurAccY, AEThreshold);
			if(valid_mode == INVALID)
			{
				valid_mode = (PreAccY - CurAccY) > EXPOSURE_RANGE ? VALID : INVALID;
				if(valid_mode == VALID)
				{
					// the next adjustment could be over the range
					// 1. the valid voltage is found
					// 2. the current voltage is too high
					// => decrease the voltage
					cal_dac_register(ENTER_VALID_MODE, valid_mode);
				}
			}
			#ifdef _IRIS_DBMSG_
			printk("\nCurAccY: %d (ccode: %d) (%d)\n", CurAccY, ccode, valid_mode);
			#endif

	

			if(ccode == EXPOSURE_IS_TOO_HIGH)
			{
				// Too brightness, begin the close procedure
				cal_dac_register(ccode, valid_mode);

				int is_next_adjustment_too_far = (CurAccY - AEThreshold < PreAccY - CurAccY) ? YES : NO;

				if(is_next_adjustment_too_far == YES)
				{
					#ifdef _IRIS_DBMSG_
					printk("SPA-REG(%d,%d)->(%d,%d)\n", 
						dac_first_register_value, dac_second_register_value, 
						dac_first_register_value - 2, dac_second_register_value - 2);
					#endif
					dac_first_register_value -= PULL_UP_VOLTAGE_IF_NEXT_ADJUSTMENT_COULD_BE_TOO_FAR;
					dac_second_register_value -= PULL_UP_VOLTAGE_IF_NEXT_ADJUSTMENT_COULD_BE_TOO_FAR;
				}

				set_dac1_register(udev, dac_first_register_value);
				udelay(20);
				set_dac1_register(udev, dac_second_register_value);

				#ifdef _IRIS_DBMSG_
				printk("Set-");
				#endif
				#ifdef _IRIS_DBMSG_
				printk("REG(%d,%d)\n", dac_first_register_value, dac_second_register_value);
				#endif
			}
			else if(ccode == EXPOSURE_IS_TOO_LOW)
			{
				// try to adjust by the gain and shutter first
				if(dac_first_register_value > 1)
				{
					dac_first_register_value -= 1;
					dac_second_register_value -= 1;
				}

				if(dac_first_register_value >= 4)
				{
					set_dac1_register(udev, dac_first_register_value - 4);
					udelay(20);
					set_dac1_register(udev, dac_second_register_value - 4);
				}
				else
				{
					set_dac1_register(udev, 0);
					udelay(20);
					set_dac1_register(udev, 6);
				}

				#ifdef _IRIS_DBMSG_
				printk("Set-REG(%d,%d)\n", dac_first_register_value - 4, dac_second_register_value - 4);
				#endif
			}
			else if(ccode == EXPOSURE_IS_VERY_LOW)
			{
				// too dark, open procedure
			}
			else
			{
				// ok, do nothing
				cal_dac_register(ccode, valid_mode);

				// reduce the tolerance range
				if(failed_and_tolerance_range > 0)
				{
					failed_and_tolerance_range--;
				}

				if(dac_first_register_value >= 4)
				{
					set_dac1_register(udev, dac_first_register_value - 4);
					udelay(20);
					set_dac1_register(udev, dac_second_register_value - 4);
				}
				else
				{
					set_dac1_register(udev, 0);
					udelay(20);
					set_dac1_register(udev, 4);
				}

				#ifdef _IRIS_DBMSG_
				printk("Set-REG(%d,%d)\n", dac_first_register_value - 4, dac_second_register_value - 4);
				#endif
			}



			if(ccode == EXPOSURE_IS_TOO_HIGH)
			{
				// if the original exposure is too high, wait one moment to see the adjusted result.
				schedule_timeout(30);

				// get the current exposure to see the change
				CurAccY = GetBulkInAvgY(udev);
				ccode = CheckCurrentAvgY(CurAccY, AEThreshold);

				#ifdef _IRIS_DBMSG_
				printk("OEH-CurAccY: %d (ccode: %d)(%d)\n", CurAccY, ccode, valid_mode);
				#endif
			}

			if(valid_mode == INVALID)
			{
				valid_mode = (PreAccY - CurAccY) > EXPOSURE_RANGE ? VALID : INVALID;
				if(valid_mode == VALID)
				{
					cal_dac_register(ENTER_VALID_MODE, valid_mode);
				}
			}

			i = 10 + failed_and_tolerance_range;
			int continue_ok = 0;

			while(((i--) > 0) && (ccode == EXPOSURE_IS_OK || ccode == EXPOSURE_IS_TOO_LOW || ccode == EXPOSURE_IS_VERY_LOW))
			{

				if(ccode == EXPOSURE_IS_OK || ccode == EXPOSURE_IS_VERY_LOW || ccode == EXPOSURE_IS_TOO_LOW)
				{
					if(failed_and_tolerance_range == 0)
					{
						#ifdef _EXP_DBMSG_
						printk("estimated range: (%d ~ )\n", dac_first_register_value + ccode * 2);
						#endif
					}
				}

				if(ccode == EXPOSURE_IS_OK)
				{
					continue_ok++;
					if(continue_ok >= 4)
					{
						// the exposure is ok, abort
						#ifdef _IRIS_DBMSG_
						printk("OK.\n");
						#endif
						break;
					}
				}
				
				if(ccode == EXPOSURE_IS_VERY_LOW)
				{
					if(CurAccY - PreAccY <= AEThreshold - CurAccY)
					{
						cal_dac_register(ccode, valid_mode);
					}

					set_dac1_register(udev, 0);
					udelay(10);
					set_dac1_register(udev, dac_second_register_value - 6);

					#ifdef _IRIS_DBMSG_
					printk("EVL-Set-REG(0,%d)\n", dac_second_register_value - 6);
					#endif
				}
				
				if(ccode == EXPOSURE_IS_TOO_LOW)
				{
					cal_dac_register(ccode, valid_mode);

					set_dac1_register(udev, dac_first_register_value - 6);
					udelay(10);
					set_dac1_register(udev, dac_second_register_value - 6);

					#ifdef _IRIS_DBMSG_
					printk("ETL-Set-REG(%d,%d)\n", dac_first_register_value - 6, dac_second_register_value - 6);
					#endif
				}

				if(ccode == EXPOSURE_IS_TOO_HIGH)
				{
					#ifdef _IRIS_DBMSG_
					printk("Too Far.. (TOLERANCE: %d)\n", failed_and_tolerance_range);
					#endif

					failed_and_tolerance_range++;
				}


				schedule_timeout(35);

				CurAccY = GetBulkInAvgY(udev);
				ccode = CheckCurrentAvgY(CurAccY, AEThreshold);

				#ifdef _IRIS_DBMSG_
				printk("YAA-CurAccY: %d (ccode: %d)(%d/+%d)\n", CurAccY, ccode, valid_mode, failed_and_tolerance_range);
				#endif	
			}
		
			PreAccY = CurAccY;
		}

		up(&sem);
		if(ccode == EXPOSURE_IS_VERY_LOW)
		{
			schedule_timeout(5);
		}
		else
		{
			schedule_timeout(1);
		}
	}

	run_thread = -1;

	printk("unload thread.\n");
}



////////////////////////////////////////////////////////////////////////////////////
// Topro Programming Subroutines
////////////////////////////////////////////////////////////////////////////////////

void topro_cam_init(struct pwc_device *pdev)    //950903
{

//  int fd,pid;
  struct usb_device *udev;    //950903
  unsigned char Data;
  printk("cam_init\n");

  udev = pdev->udev;          //950903
  BitmapWidth = 640;   //640, 320, 176
  BitmapHeight = 480;  //480, 240, 144 
  FrameRate=30;    //by user define
  Frequency=60;  //by user selected (indoor ~ 50 or 60 , outdoor ~ 0)
  ImageQuality = 0x0f;    // 0 ~ 15, 0 is best
  Motion_Ythrld = 0x00; //Software detect motion
  HwMotion_Ythrld = 0x00;// Hw motion detection Y threshold

#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 [(init en-autoexposure, en-AWB)]
  AutoExposure=1;
  AutoWhiteBalance=1;
#elif defined TP6830_TICCD
	
	topro_freq_control();
	topro_TI_MaxExpo();
//;AutoExposure=0; printk("TP6830:: Cam_Init :: Dis AE, Current Exposure= %d \n", CurrentExposure);
   AutoExposure=1; 
//;AutoExposure=0; 
   AutoWhiteBalance=1;
	
#else
  AutoExposure=1;
  AutoWhiteBalance=1;
#endif

  AutoImageQuality=1;
#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 [(init not show OSD, NOT SHOW RTC)]
  ShowOSD = 0;
  ShowRTC = 0;
#elif defined TP6830_TICCD
  ShowOSD = 0;
  ShowRTC = 0;
#else
  ShowOSD = 1;
  ShowRTC = 1;
#endif

  pdev->exposurelevel = 10;		//950903

  pdev->awbmode = PWC_WB_AUTO; //950903

 	topro_write_reg(udev, MCLK_SEL, 0x11);
	topro_write_reg(udev, CLK_CFG, 0x7a);
	topro_write_reg(udev, PCLK_CFG, 0x25);   // SREG= PCLK_CFG= 04h

	topro_write_reg(udev, ISP_MODE, 0x00);
	topro_write_reg(udev, MCLK_CFG, 0x03);   // SREG= MCLK_CFG= 01h

	//Reset Sensor High->Low->High

	topro_write_reg(udev, GPIO_PU, 0xef);
	topro_write_reg(udev, GPIO_PD, 0xc0);

	 //;   topro_write_reg(udev, GPIO_IO, 0x97);  //9f
	 //;   topro_write_reg(udev, GPIO_DATA, 0xC1); //c1
	
	topro_write_reg(udev, /*GPIO_DIR:0x1B*/GPIO_IO, 0x17);  // 增加 bit7 out
																													// 原: bit6 out
																													//     bit5 out
																													//     bit3 out
	topro_write_reg(udev, GPIO_DATA, 0x01); // 0x41<--c1 ,  bit7 out is init low ("{GPIO_XXX_NUM, GPIO_STATUS_LOW}")
																					//              bit6 out is init low (refer to "struct gpio_status gs= {GPIO_IR_FILTER_NUM, GPIO_STATUS_LOW}")
																					//              bit5 out is init low ("{XXX, GPIO_STATUS_LOW}")
																					//              bit3 out is init low ("{XXX, GPIO_STATUS_LOW}")
/*  topro_write_reg(udev, GAGEN_CTL, 0x04);
  topro_write_reg(udev, GAMMA_ADDR_L, 0x00);
  topro_write_reg(udev, GAMMA_ADDR_H, 0x00);

	if (!topro_prog_gamma(pdev, 0x00))  //950903
		printk("program gamma R OK.\n");

	if (!topro_prog_gamma(pdev, 0x01))  //950903
		printk("program gamma G OK.\n");

  if (!topro_prog_gamma(pdev, 0x02))  //950903
		printk("program gamma B OK.\n");

  topro_write_reg(udev, GAGEN_CTL, 0x00);
  topro_write_reg(udev, GAMMA_ADDR_L, 0x00);
  topro_write_reg(udev, GAMMA_ADDR_H, 0x00);

	if (!topro_prog_gamma(pdev, 0x00))    //950903
		printk("program gamma R OK.\n");

	if (!topro_prog_gamma(pdev, 0x01))    //950903
		printk("program gamma G OK.\n");

  if (!topro_prog_gamma(pdev, 0x02))    //950903
		printk("program gamma B OK.\n");

  topro_write_reg(udev, GAGEN_CTL, 0x04);
*/
	topro_setting_iso(udev, 0x00);
    	topro_write_reg(udev, GAMMA_ADDR_L, 0x00);
	topro_write_reg(udev, GAMMA_ADDR_H, 0x00);
	
	if (!topro_prog_gamma(pdev, 0x00))  //950903
		printk("program gamma R OK.\n");
	
	if (!topro_prog_gamma(pdev, 0x01))  //950903
		printk("program gamma G OK.\n");
	
	if (!topro_prog_gamma(pdev, 0x02))  //950903
		printk("program gamma B OK.\n");
	
	Data = topro_read_reg(udev, GAGEN_CTL);
	if(Data & 0x04){
		Data &= 0xfb;
		Data |= 0x01;
	 	topro_write_reg(udev, GAGEN_CTL, Data);
	}
	else{
		Data |= 0x05;
		topro_write_reg(udev, GAGEN_CTL, Data);
	}
	
	topro_setting_iso(udev, 0x00);
	
      	if (BitmapWidth == 640){	
	    	jpg_hd[509] = 0x01;
	    	jpg_hd[510] = 0xe0;
	    	jpg_hd[511] = 0x02;
	    	jpg_hd[512] = 0x80;
      	}
      	else if (BitmapWidth == 320){
	    	jpg_hd[509] = 0x00;
	    	jpg_hd[510] = 0xf0;
	    	jpg_hd[511] = 0x01;
	    	jpg_hd[512] = 0x40;
      	}
      	else{        //176
	    	jpg_hd[509] = 0x00;
	    	jpg_hd[510] = 0x90;
	    	jpg_hd[511] = 0x00;
	    	jpg_hd[512] = 0xb0;
      	}

  
#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 (init)

	if (topro_prog_regs(udev, init_Micron_MT9M001))  // tp68xx initialize data
		printk("program initial data error.\n");
	

  if(BitmapWidth == 640){   // VGA parameter

	  if (topro_prog_regs(udev, vga_micron_MT9M001_vga))  //930406
		  printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);        

  }
  //; else if(BitmapWidth == 176){  // TP6830_QCIF		//;[parameter]
	//;   if (topro_prog_regs(udev, qcif_micron_MT9M001_qcif))  //951011 JJ--
	//; 	  printk("program qcif data error.\n");
  //;   topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  //; }
  else if(BitmapWidth == 176){  // TP6830_QCIF		//;[parameter]
		printk("1.cam_init, 176/qcif registers\n");
	   if (topro_prog_regs(udev, qcif_micron_MT9M001_qcif))  //951011 JJ--
	 	  printk("program qcif data error.\n");
     topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
		printk("2.scaling down horizontal&vertical \n");
		printk("2.cam_init, 176/qcif registers\n");
  }
  else{  // QVGA parameter
	  if (topro_prog_regs(udev, qvga_micron_MT9M001_qvga))  //930406
		  printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  }

	if (topro_prog_regs(udev, i2c_Micron_MT9M001_I2C)) 	// initialize sensor
		printk("program sensor error.\n");

#elif defined(TP6830_MT9V111) // micron TP6830_MT9V111 ;(ISP,VGA)

  // tp68xx initialize data
  printk("TP6830_MT9V111\n");
	if (topro_prog_regs(udev, topro_init_micron_MT9V111))

		printk("program initial data error.\n");

  if(BitmapWidth == 640){

  // VGA parameter
	  if (topro_prog_regs(udev, topro_vga_micron_MT9V111))  //930406
		  printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);

  }
  else if(BitmapWidth == 176){ //TP6830_QCIF
	  if (topro_prog_regs(udev, topro_qcif_micron_MT9V111))  //930406
		  printk("program qcif data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical

  }

  else{
	  if (topro_prog_regs(udev, topro_qvga_micron_MT9V111))  //930406
		  printk("program vga data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical

  }
		Data = topro_read_reg(udev, GPIO_DATA);
		Data |= 0x20;
		topro_write_reg(udev, GPIO_DATA, Data);
		udelay(1000);
		Data &= 0xdf;
		topro_write_reg(udev, GPIO_DATA, Data);
		udelay(2000);
		Data |= 0x20;
		topro_write_reg(udev, GPIO_DATA, Data);
		udelay(1000);
      
	// initialize sensor
	if (topro_prog_regs(udev, topro_i2c_micron_MT9V111))
		printk("program sensor error.\n");    

#elif defined(TP6830_MT9V011) // micron MT9V011

  // tp68xx initialize data
	if (topro_prog_regs(udev, topro_init_micron_MT9V011))
		printk("program MT9V011 initial data error.\n");


  if(BitmapWidth == 640){
  // VGA parameter
	  if (topro_prog_regs(udev, topro_vga_micron_MT9V011))  //930406
		  printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);        

  }
  else if(BitmapWidth == 176){ //TP6830_QCIF
	  if (topro_prog_regs(udev, topro_qcif_micron_MT9V011))  //930406
		  printk("program qcif data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical


  }
  else{
	  if (topro_prog_regs(udev, topro_qvga_micron_MT9V011))  //930406
		  printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical


  }

	// initialize sensor
	if (topro_prog_regs(udev, topro_i2c_micron_MT9V011))
		printk("program sensor error.\n");

#elif defined TP6830_SHARPCCD

  // tp68xx initialize data
	if (topro_prog_regs(udev, Sharp_CCD))
		printk("program initial data error.\n");



  if(BitmapWidth == 640){
  // VGA parameter

  	if (topro_prog_regs(udev, Sharp_CCD_VGA))  //930406

	  	printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);  

  }

  else if(BitmapWidth == 176){    //TP6830_QCIF
  // VGA parameter
  	if (topro_prog_regs(udev, Sharp_CCD_QCIF))  //930406

	  	printk("program vga data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  }

  else{

  // QVGA parameter
  	if (topro_prog_regs(udev, Sharp_CCD_QVGA))  //930406

	  	printk("program qvga data error.\n");


    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  }

	// initialize sensor

	if (topro_prog_regs(udev, Sharp_CCD_I2C))
		printk("program sensor error.\n");


#elif defined TP6830_SONYCCD

  // tp68xx initialize data
	if (topro_prog_regs(udev, Sony_CCD))
		printk("program initial data error.\n");




  if(BitmapWidth == 640){
  // VGA parameter
  	if (topro_prog_regs(udev, Sony_CCD_VGA))  //930406
	  	printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);  



  }
  else if(BitmapWidth == 176){
  	if (topro_prog_regs(udev, Sony_CCD_QCIF))  //930406
	  	printk("program qvga data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical

  }  
  else{
  // QVGA parameter
  	if (topro_prog_regs(udev, Sony_CCD_QVGA))  //930406
	  	printk("program qvga data error.\n");
     
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  }

	// initialize sensor
	if (topro_prog_regs(udev, Sony_CCD_I2C))
		printk("program sensor error.\n");

#elif defined(TP6830_TICCD)  	// TI TP6830_TICCD (init)

  // tp68xx initialize data
  printk("program TI CCD parameters. \n");
	if (topro_prog_regs(udev, TI_CCD))
		printk("program initial data error.\n");

  if(BitmapWidth == 640){
  // VGA parameter
  printk("program TI CCD VGA parameters. \n");
  	if (topro_prog_regs(udev, TI_CCD_VGA))  //930406
	  	printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);
  }
  else{
  // QVGA parameter
  printk("program TI CCD QVGA parameters. \n");
  	if (topro_prog_regs(udev, TI_CCD_QVGA))  //930406
	  	printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  }

		Data = topro_read_reg(udev, GPIO_DATA);
		Data |= 0x20;
		topro_write_reg(udev, GPIO_DATA, Data);
		udelay(10); //udelay(1000);
		Data &= 0xdf;
		topro_write_reg(udev, GPIO_DATA, Data);
		udelay(10); //udelay(2000);
		Data |= 0x20;
		topro_write_reg(udev, GPIO_DATA, Data);
		udelay(10); //udelay(1000);

	//; TI_SPI_FREESPD
  //;	TI_SPI_HISPEED
	if (!topro_write_reg(udev, CLK_CFG, TI_SPI_HISPEED))
		printk("TI CLK CFG HiSpeed OK \n");
    
		// initialize sensor
		printk(" program TI CCD sensor preSPI16 parameters. \n");
		//;if (topro_prog_regs(udev, TI_CCD_SPI16_PRE))
			 if (topro_prog_spi16(udev, SPI16_PRE))
				printk("pre-program sensor error.\n");

		//udelay(10);
		printk(" program TI CCD sensor SPI32 parameters. Chap1 \n");
		//;if (topro_prog_regs(udev, TI_CCD_I2C))
			 if (topro_prog_spi32(udev, SPI32_I2C_Chap1))
				printk("Spi32 program sensor error.\n");
			 
			//udelay(10);
			printk(" program TI CCD sensor SPI32 parameters. Chap1e \n");
				 if (topro_prog_spi32(udev, SPI32_I2C_Chap1e))
					printk("Spi32 program sensor error.\n");
			//udelay(10);
			printk(" program TI CCD sensor SPI32 parameters. Chap2 \n");
				 if (topro_prog_spi32(udev, SPI32_I2C_Chap2))
					printk("Spi32 program sensor error.\n");

			//udelay(10);
			printk(" program TI CCD sensor SPI32 parameters. Chap5 \n");
				 if (topro_prog_spi32(udev, SPI32_I2C_Chap5))
					printk("Spi32 program sensor error.\n");

		//udelay(10);
		printk(" program TI CCD sensor postSPI16 parameters. \n");
		//;if (topro_prog_regs(udev, TI_CCD_SPI16_POST))
			 if (topro_prog_spi16(udev, SPI16_POST))
				printk("post-program sensor error.\n");

	udelay(1);
	if (!topro_write_reg(udev, CLK_CFG, TI_SPI_NORMSPD))
		printk("TI CLK CFG Normal-Speed OK \n");

#endif

#if 1
  //; Gain - Always by (already inside) 'topro_set_hue_saturation()'
	//   if (_topro_prog_gain(udev, new_gain)) printk("program gain error.\n");
		  topro_prog_gain(udev, new_gain);
#else
			topro_set_hue_saturation(udev, topro_hue, topro_saturation);  // _topro_prog_gain() insided
#endif

	// set frame rate

#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 (init), [(SetFrameRate), MCLK_SEL,MCLK_CFG]

	if (FrameRate != 1)
    Data = 0x01;
	else
    Data = 0x03;

	topro_write_reg(udev, MCLK_SEL, 0x10 | Data);  // DBG_4_MT9M001 (bit4 set)
	topro_write_reg(udev, MCLK_CFG, 0x03);  
 

#elif defined(TP6830_MT9V011)

	if (FrameRate != 1)
    Data = 0x01;
	else
    Data = 0x03;

	topro_write_reg(udev, MCLK_SEL, Data);
	topro_write_reg(udev, MCLK_CFG, 0x03);  
 
//  if(BitmapWidth == 640)
//    topro_write_i2c(udev, 0x5d, 0x0a, 0x00, 0x02);  //30fps
//  else  
//    topro_write_i2c(udev, 0x5d, 0x0a, 0x00, 0x00);  //15fps

#elif defined(TP6830_SHARPCCD) || defined(TP6830_SONYCCD)

#ifdef ISSC
   topro_write_reg(udev, MCLK_SEL, 0x03);
   topro_write_reg(udev, MCLK_CFG, 0x03);
#else   
   topro_write_reg(udev, MCLK_SEL, 0x03);
   topro_write_reg(udev, MCLK_CFG, 0x03);
#endif   

#elif defined(TP6830_TICCD)  	// TI TP6830_TICCD (init), [(SetFrameRate), MCLK_SEL,MCLK_CFG]

		#if CHG_4_MT9M001

		 D.....,mnfsmbjkebv THIS IS REALLY FOR TI CCD PROJECT ...jkmnbhjbsvhjvhjvjw.

			 if (FrameRate != 1)
				Data = 0x01;
			 else
				Data = 0x03;
			 topro_write_reg(udev, MCLK_SEL, Data);

		#else

		 topro_write_reg(udev, MCLK_SEL, 0x11);
		#endif

    topro_write_reg(udev, MCLK_CFG, 0x03);
   
#elif defined(TP6830_MT9V111)

  if (FrameRate == 30)
    Data = 0x01;
  else if (FrameRate == 20)
    Data = 0x02;

  else if (FrameRate == 15)
    Data = 0x11;
  else
    Data = 0x04;

   topro_write_reg(udev, MCLK_SEL, Data);
   topro_write_reg(udev, MCLK_CFG, 0x03);

#endif

	topro_fps_control(udev, 0, CurrentExposure);
	topro_setexposure(udev, CurrentExposure,pdev);

	topro_setting_iso(udev, 0x01);

	topro_setquality(udev, ImageQuality);

#ifdef CUSTOM_QTABLE  
  UpdateQTable(udev, 1);					//steven 950111
//  topro_write_reg(udev, 0x7d, 0x08); //reset qtable state
#endif

  topro_set_edge_enhance(udev);

  if(HwMotion_Ythrld){
    topro_write_reg(udev, MOTION_YTH_L, (HwMotion_Ythrld & 0xff));
    topro_write_reg(udev, MOTION_YTH_H, ((HwMotion_Ythrld >> 8 )& 0xff));    
  }

//////////////////////////
//OSD function
//////////////////////////
#if 0
	if(ShowOSD)
		topro_set_osd(udev,ShowOSD);
	else
#endif
	topro_set_osd(udev,0);    
	topro_config_rtc(udev);
#if 0
	if(ShowRTC){
		Rtc_Data.year = 2006;
		Rtc_Data.month = 10;
		Rtc_Data.day = 20;
		Rtc_Data.hour = 10;
		Rtc_Data.minute = 30;
		Rtc_Data.second = 30;
		update_rtc_time(udev, &Rtc_Data);
	}
#endif

	topro_write_reg(udev, AUTOQ_FUNC, 0x40);
             
	//List_CCDExposure();
	sema_init(&sem, 1);

	// run the kernel to adjust the awb & aec (+auto-iris)
	kernel_thread(topro_iris_thread, pdev, (CLONE_FS | CLONE_FILES | CLONE_SIGHAND));

	//printk("_HZ_: %d \n", HZ);
	//printk("USB_CTRL_SET_TIMEOUT: %d \n", USB_CTRL_SET_TIMEOUT /*= HZ*2*/ );

	printk("Initialize Topro Camera OK [1222-001] - Kenerel Thread!!\n");
}




////////////////////////////////////////////////////////////////////////////////
// Read topro register
////////////////////////////////////////////////////////////////////////////////
unsigned char topro_read_reg(struct usb_device *udev, unsigned char index)
{
	unsigned short buf = 0x0000;

	unsigned char data;

	int ret;


	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),

		READ_REGISTER,
		READ_REGISTER_TYPE,
		0x0000,
		(buf<< 8) + index,
		&data, 1, 
		
		USB_CTRL_SET_TIMEOUT /*HZ*/);

	if (ret<0){
		printk("TIMEOUT: topro_read_reg %d \n", ret);
		return data; // ? ret;
	}
	return data;
}


////////////////////////////////////////////////////////////////////////////////
// write topro i2c
////////////////////////////////////////////////////////////////////////////////

int topro_write_i2c(struct usb_device *udev, unsigned char dev, unsigned char reg, unsigned char data_h, unsigned char data_l)
{
#if defined(TP6830_SHARPCCD) || defined(TP6830_SONYCCD)
	topro_write_reg(udev, SIF_ADDR_S2, dev);
	topro_write_reg(udev, SIF_TX_DATA1, reg);
	topro_write_reg(udev, SIF_TX_DATA2, data_h);

	topro_write_reg(udev, SIF_TX_DATA3, data_l);



	topro_write_reg(udev, SIF_CONTROL, 0x01);

//  printk("I2C CCD %x %x %x %x\n",dev, reg, data_h,data_l);
#else
#ifdef TP6830_MT9V111

  if (dev != 0x5c)
    return 1;
#endif  
	topro_write_reg(udev, SIF_ADDR_S1, dev);

	topro_write_reg(udev, SIF_ADDR_S2, reg);
	topro_write_reg(udev, SIF_TX_DATA1, data_h);
	topro_write_reg(udev, SIF_TX_DATA2, data_l);

	topro_write_reg(udev, SIF_CONTROL, 0x01);
//  printk("write I2C %x %x %x %x\n",dev, reg, data_h,data_l);

#endif
 	return 1;
}

int topro_write_spi16(struct usb_device *udev, unsigned char dev, unsigned char reg)
{
	topro_write_reg(udev, SIF_ADDR_S2, dev);  // _spi16: 0x9e/ _spi16: 0x9f/ _spi16: 0x07, 
	topro_write_reg(udev, SIF_TX_DATA1, reg);

//;topro_write_reg(udev, SIF_CONTROL, 0x04); JJ- ;Dbg 20061019
	topro_write_reg(udev, SIF_CONTROL, 0x01);

//  printk("I2C CCD %x %x %x %x\n",dev, reg, data_h,data_l);
 	return 0;  // No Error
}

int topro_write_spi32(struct usb_device *udev, unsigned char dev, unsigned char reg1,
												unsigned char reg2, unsigned char reg3)
{
	//;{SIF_ADDR_S2,0x00},{SIF_TX_DATA1,0x5a},{SIF_TX_DATA2,0x80},{SIF_TX_DATA3,0x0c},{SIF_CONTROL,0x01},
	topro_write_reg(udev, SIF_ADDR_S2, dev);
	topro_write_reg(udev, SIF_TX_DATA1, reg1);
	
	topro_write_reg(udev, SIF_TX_DATA2, reg2);
	topro_write_reg(udev, SIF_TX_DATA3, reg3);
	
	topro_write_reg(udev, SIF_CONTROL, 0x01);
 	return 0;  // No Error
}

int topro_write_i2c_ccd(struct usb_device *udev, unsigned char dev, unsigned char reg, unsigned char data_h, unsigned char data_l)

{

	topro_write_reg(udev, SIF_ADDR_S2, dev);
	topro_write_reg(udev, SIF_TX_DATA1, reg);

	topro_write_reg(udev, SIF_TX_DATA2, data_h);
	topro_write_reg(udev, SIF_TX_DATA3, data_l);
	topro_write_reg(udev, SIF_CONTROL, 0x04);


//  printk("I2C CCD %x %x %x %x\n",dev, reg, data_h,data_l);
 	return 1;

}

//===================================================================
//      topro_ReadI2C
//===================================================================


int topro_read_i2c(struct usb_device *udev, unsigned char dev, unsigned char reg, unsigned char *data_h, unsigned char *data_l)
{


  topro_write_reg(udev, SIF_ADDR_S1, dev);
	topro_write_reg(udev, SIF_ADDR_S2, reg);
	topro_write_reg(udev, SIF_CONTROL, 0x02);  
	*data_h=topro_read_reg(udev, SIF_RX_DATA1);

	*data_l=topro_read_reg(udev, SIF_RX_DATA2);

  printk("read I2C  %x %x %x %x\n",dev, reg, *data_h,*data_l);

 	return 1;

}


////////////////////////////////////////////////////////////////////////////////
// write topro register
////////////////////////////////////////////////////////////////////////////////

int topro_write_reg(struct usb_device *udev, unsigned char index, unsigned char data)

{
	int ret;
	unsigned short buf = 0x0000;
//  if(index < 0x11 || index > 0x18 )
//    printk("write reg %x %x \n",index, data);
  
	ret= usb_control_msg(

		udev,								// usb_dev_handle
		usb_sndctrlpipe(udev, 0),				// request type
		WRITE_REGISTER,							// request
		WRITE_REGISTER_TYPE,				// value

		(buf<< 8) + data,						// index
		(buf<< 8) + index,					// *bytes

		NULL,									// size
		0,			
		
		USB_CTRL_SET_TIMEOUT /*HZ*/);				// timeout {USB_CTRL_SET_TIMEOUT (another constant)}

	if (ret<0){
		printk("TIMEOUT: topro_write_reg %d \n", ret);
		return ret;
	}
	return 0;
}



////////////////////////////////////////////////////////////////////////////////
// program topro registers table
////////////////////////////////////////////////////////////////////////////////

int topro_prog_spi16(struct usb_device *udev, unsigned char *pTab)
{
	unsigned char Addr;
	unsigned char Dat;

	topro_write_reg(udev, SIF_TYPE, 0x01); // SPI16 啟始

	while (1)
	{
		Addr= *pTab++;
		Dat= *pTab++;

		if (Addr==END_REG && Dat==END_REG)
			break;
		if (Addr==END_REG && Dat==0){
      			printk("Program spi16 table end-finish\n");
			break;
		}

		topro_write_spi16(udev, Addr, Dat);

		if (WAIT_SIF(udev)){
        		printk("Program spi16 table error, SPI No Ack\n");
			return 1; // Error
		}
	}
	return 0; // No Error
}

int topro_prog_spi32(struct usb_device *udev, unsigned char *pTab32)
{
	unsigned char Addr;
	unsigned char Dat1,Dat2,Dat3;

	topro_write_reg(udev, SIF_TYPE, 0x03); // SPI32 啟始

	while (1)
	{
		Addr= *pTab32++;
		Dat1= *pTab32++;
		if (Addr==END_REG && Dat1==END_REG)
			break;
		if (Addr==END_REG && Dat1==0){
      			printk("Program spi32 table end-finish\n");
			break;
		}
		Dat2= *pTab32++;
		Dat3= *pTab32++;

	//;20061114-JJ
		if (Dat2==0 && Dat3==0)
			continue;
		//;if (Dat2==0 && Dat3==0)
		//;{
		//;	if (Dat1==1 || Dat1==2 || Dat1==3)
		//;		continue;
		//;	printk("Program spi32 table warning! Not H100~H3FF, %x, %x, %x, %x \n", Addr, Dat1, Dat2, Dat3);
		//;}
	//;
		topro_write_spi32(udev, Addr, Dat1,Dat2,Dat3);
		if (WAIT_SIF(udev)){
			printk("Program spi32 table error, SPI No Ack\n");
			return 1; // Error
		}
	}

	return 0; // No Error
}

int topro_prog_regs(struct usb_device *udev, struct topro_sregs *ptpara)
{
	unsigned short index= 0;
	int ret= 0, tx_num= 0;
	int i;

	do
	{
		if (ptpara[index].index == END_REG) // check end of table
			break;

		// _USB control transfer
		ret |= topro_write_reg(udev, ptpara[index].index, ptpara[index].value);

    if (ptpara[index].index==SIF_CONTROL)
		{
      //;for (i=0;i<100;i++) {
      //;  if (topro_read_reg(udev, SIF_CONTROL)==0x00)
      //;    break;        
      //;}
			tx_num++;
			if (i= WAIT_SIF(udev))
			{
        printk("(%d) Program sensor error, SPI No Ack\n", tx_num);
        break;
			}
		}

    index++; // next
	} while(index);

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int topro_setting_iso(struct usb_device *udev, unsigned char enset)
{



	int ret = 0;


	if (enset) // enable

	{

		// open isochronous


		ret |= (topro_write_reg(udev, ENDP_1_CTL, 0x03));
	}
	else // disable
	{
		// close isochronous

		ret |= (topro_write_reg(udev, ENDP_1_CTL, 0x00));

	}




	return ret;

}


////////////////////////////////////////////////////////////////////////////////
// USB bulk out function
////////////////////////////////////////////////////////////////////////////////

#define TOPRO_BULK_LENGTH 31

int topro_bulk_out(struct usb_device *udev, unsigned char bulkctrl, unsigned char *pbulkdata, int bulklen)
{
	unsigned char BulkBuffer[TOPRO_BULK_LENGTH + 1];  // [31+1]
	int RetLen;
	int index;
	int fullloop, remainloop;
	int ret = 0;

			//	topro_setting_iso(udev, 0);
	//-------------------------------- close isochronous

			//  topro_write_reg(udev, 0x02, 0x28);

				BulkBuffer[0] = bulkctrl; // set control byte
				fullloop = bulklen / TOPRO_BULK_LENGTH;
				remainloop = bulklen % TOPRO_BULK_LENGTH;

				// full part
				for (index=0; index<fullloop; index++)
				{
				 memcpy(BulkBuffer + 1, pbulkdata + (index * TOPRO_BULK_LENGTH), TOPRO_BULK_LENGTH);
				 ret |= usb_bulk_msg(udev, usb_sndbulkpipe(udev, 3), BulkBuffer, TOPRO_BULK_LENGTH + 1, &RetLen, HZ);

				} //	 printk("Topro Bulk Status(0) = %d Act = %d \n", ret, RetLen);

				// remain part
				if (remainloop)
				{
				 memcpy(BulkBuffer + 1, pbulkdata + (index * TOPRO_BULK_LENGTH), remainloop);
				 ret |= usb_bulk_msg(udev, usb_sndbulkpipe(udev, 3), BulkBuffer, remainloop + 1, &RetLen, HZ);

				} //	 printk("Topro Bulk Status(1) = %d Act = %d \n", ret, RetLen);

	//--------------------------------- open isochronous
			//	topro_setting_iso(udev, 1);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////


// USB bulk in function
////////////////////////////////////////////////////////////////////////////////


int topro_bulk_in(struct usb_device *udev, unsigned char bulkctrl, unsigned char *pbulkdata, int bulklen)
{

	int ret = 0;


  int RetLen;


  ret |= usb_bulk_msg(udev, usb_rcvbulkpipe(udev, 2), pbulkdata, bulklen, &RetLen, HZ);
  
	return ret;
}





////////////////////////////////////////////////////////////////////////////////
// Get CRC value

////////////////////////////////////////////////////////////////////////////////
#define _CRC_G_ 0x8005


unsigned short tp_get_CRC(unsigned char *pbuffer, int bffsize)

{
	int i, j; // loop index

	unsigned char nextbff;


	unsigned short hold, hold_bff;

	hold = 0xffff;

	for (i=0; i<bffsize; i++)
	{
		for (j=0; j<8; j++)


		{
			nextbff = (pbuffer[i] & (0x01 << j)) >> j;
			hold_bff = hold;
			hold = (hold_bff << 1) & 0xfffe;


			if (nextbff != ((hold_bff & 0x8000) >> 15))
				hold = hold ^ _CRC_G_;
		} // for
	} // for


	return ~hold;
}



////////////////////////////////////////////////////
// Calculate CRC Of Gamma Block
////////////////////////////////////////////////////
void tp_cal_CRC(unsigned char bulkctrl, unsigned char *pGammaBlock, int blocklen)





{
	int index;

	unsigned short CRCvalue;

	unsigned char BulkBuffer[TOPRO_BULK_LENGTH + 1];

	BulkBuffer[0] = bulkctrl;
	while(1)


	{
		memcpy(BulkBuffer + 1, pGammaBlock, blocklen);

		CRCvalue = tp_get_CRC(BulkBuffer, blocklen + 1);

	 	if ((((unsigned char)(CRCvalue >> 8) & 0x3F) == 0x3F) ||

			(((unsigned char)CRCvalue & 0x3F) == 0x3F))
		{
			for (index=0; index<blocklen; index++)
				if (pGammaBlock[index] != 0) break;


			if (index == blocklen) // if all items are zeros, infinite loop
				pGammaBlock[index - 1]++;



			else
				pGammaBlock[index]--;




			continue; // next

		} // if
		else

			break;



	} // while
}



////////////////////////////////////////////////////////////////////////////////

// Programming gamma function (set default gamma)
////////////////////////////////////////////////////////////////////////////////
//950903
int topro_prog_gamma(struct pwc_device *pdev, unsigned char bulkctrl)
{
	int i;
  unsigned char *pGamma;
  int status;
 

  unsigned char gamma1[256], gamma2[1024], gamma_6830[512];
  switch (bulkctrl) // select gamma table
	{
	 case 0x00:
		if (pdev->awbmode == PWC_WB_OUTDOOR || pdev->awbmode == PWC_WB_FL)
			  pGamma = tp_gamma2_r;
		else if (pdev->awbmode == PWC_WB_INDOOR || pdev->awbmode == PWC_WB_FL_Y)
			  pGamma = tp_gamma3_r;
		else if (pdev->awbmode == PWC_WB_BW)
			  pGamma = tp_gamma_r;
		else //PWC_WB_AUTO
			  pGamma = tp_gamma2_r;

		break;
	 case 0x01:
		if (pdev->awbmode == PWC_WB_OUTDOOR || pdev->awbmode == PWC_WB_FL)
			  pGamma = tp_gamma2_g;
		else if (pdev->awbmode == PWC_WB_INDOOR || pdev->awbmode == PWC_WB_FL_Y)
			  pGamma = tp_gamma3_g;
		else if (pdev->awbmode == PWC_WB_BW)
			  pGamma = tp_gamma_g;
		else //PWC_WB_AUTO
			  pGamma = tp_gamma2_g;

		break;

	 case 0x02:
		if (pdev->awbmode == PWC_WB_OUTDOOR || pdev->awbmode == PWC_WB_FL)
			  pGamma = tp_gamma2_b;
		else if (pdev->awbmode == PWC_WB_INDOOR || pdev->awbmode == PWC_WB_FL_Y)
			  pGamma = tp_gamma3_b;
		else if (pdev->awbmode == PWC_WB_BW)
			  pGamma = tp_gamma_b;
		else //PWC_WB_AUTO
			  pGamma = tp_gamma2_b;

		break;    

	 default:
		return -1; // fail
	} // switch



   CalculateCBG(gamma1, topro_contrast, topro_brightness, topro_gamma);
//   printk("CalculateCBG %d %d %d\n", topro_contrast, topro_brightness, topro_gamma);

   for (i=0;i<1024;i++){
     gamma2[i] = gamma1[pGamma[i]];

   }

 //in tp6830, gamma has been change to 9 bits
  for(i=0;i<GAMMA_TABLE_LENGTH_6830;i++){  
    gamma_6830[i] = gamma2[i*2];
  }

//  topro_setting_iso(pdev->udev, 0x00);
    status = topro_bulk_out(pdev->udev, bulkctrl, gamma_6830, GAMMA_TABLE_LENGTH_6830); //950903


         
//  topro_setting_iso(pdev->udev, 0x01);
        

	return status;


}
int topro_prog_gamma1(struct pwc_device *pdev, unsigned char bulkctrl)
{
	int i;
  unsigned char *pGamma;
  int status;
 

  unsigned char gamma1[256], gamma2[1024], gamma_6830[512];
  switch (bulkctrl) // select gamma table
	{
	 case 0x00:
		if (pdev->awbmode == PWC_WB_OUTDOOR || pdev->awbmode == PWC_WB_FL)
			  pGamma = tp_gamma21_r;
		else if (pdev->awbmode == PWC_WB_INDOOR || pdev->awbmode == PWC_WB_FL_Y)
			  pGamma = tp_gamma3_r;
		else if (pdev->awbmode == PWC_WB_BW)
			  pGamma = tp_gamma_r;
		else //PWC_WB_AUTO
			  pGamma = tp_gamma2_r;

		break;
	 case 0x01:
		if (pdev->awbmode == PWC_WB_OUTDOOR || pdev->awbmode == PWC_WB_FL)
			  pGamma = tp_gamma21_g;
		else if (pdev->awbmode == PWC_WB_INDOOR || pdev->awbmode == PWC_WB_FL_Y)
			  pGamma = tp_gamma3_g;
		else if (pdev->awbmode == PWC_WB_BW)
			  pGamma = tp_gamma_g;
		else //PWC_WB_AUTO
			  pGamma = tp_gamma2_g;

		break;

	 case 0x02:
		if (pdev->awbmode == PWC_WB_OUTDOOR || pdev->awbmode == PWC_WB_FL)
			  pGamma = tp_gamma21_b;
		else if (pdev->awbmode == PWC_WB_INDOOR || pdev->awbmode == PWC_WB_FL_Y)
			  pGamma = tp_gamma3_b;
		else if (pdev->awbmode == PWC_WB_BW)
			  pGamma = tp_gamma_b;
		else //PWC_WB_AUTO
			  pGamma = tp_gamma2_b;

		break;    

	 default:
		return -1; // fail
	} // switch



   CalculateCBG(gamma1, topro_contrast, topro_brightness, topro_gamma);
//   printk("CalculateCBG %d %d %d\n", topro_contrast, topro_brightness, topro_gamma);

   for (i=0;i<1024;i++){
     gamma2[i] = gamma1[pGamma[i]];

   }

 //in tp6830, gamma has been change to 9 bits
  for(i=0;i<GAMMA_TABLE_LENGTH_6830;i++){  
    gamma_6830[i] = gamma2[i*2];
  }

//  topro_setting_iso(pdev->udev, 0x00);
    status = topro_bulk_out(pdev->udev, bulkctrl, gamma_6830, GAMMA_TABLE_LENGTH_6830); //950903


         
//  topro_setting_iso(pdev->udev, 0x01);
        

	return status;


}


//===================================================================
//      CalculateCBG()
//===================================================================

void CalculateCBG(unsigned char *pData, int Contrast, int Brightness, int Gamma)

{
//	int i, j, B, C, G, T, A, Ln_A;
	int i, j, B, C, G, T, Ln_A;
  long L;

	unsigned char DataBuf[256];
	int pstart, pend;
	unsigned char *Src, *Dst;


    B = max(Brightness,0);
    B = min(B,100);
    C = max(Contrast,0);
    C = min(C,100);

    G = max(Gamma,10);
    G = min(G,30);

    G = 1000* 10000 / (G * (50 + B));			//	0.5 < G / 1000 < 4.5



																					//	1/4.5 < 1/(G / 10000) < 1/0.5

    for (i=0;i<256;i++){

		j=198;
		Ln_A = Ln256[i] * G /10000;
		while ((j > 0) && (Ln_A_Table[j] < Ln_A))
			j-=2;

	    T = Ln_A_Table[j+1];			// 0.0521 * 10000 < A_Table < 0.9992 * 10000


		T = (T * (50 + C) * 256) / (1000);
		L = (long)((T + 50) / 100);  // 四捨五入
		*(DataBuf+i) = min(L,(long)255);
		*(DataBuf+i) = max(*(DataBuf+i),(UCHAR)0);
	}


	Src = DataBuf;
	Dst = pData;
	pstart = 0;
	pend = 1;


	while (pend<255){




		while ((Src[pend] == Src[pstart]) && (pend < 256))
			pend++;
		if (pend < 256){
			Dst[pstart] = Src[pstart];



			if (pend != pstart + 1){

				for (j=1;j<=pend-pstart;j++)
					Dst[pstart+j] = Src[pstart] + (Src[pend] - Src[pstart]) * j / (pend - pstart);
			}

			Dst[pend] = Src[pend];

			pstart = pend;
			pend++;

		}
		else
			for (j=pstart;j<256;j++)
				Dst[j] = Src[j];
	}



}





////////////////////////////////////////////////////////////////////////////////





// Programming R/G/B gain function
////////////////////////////////////////////////////////////////////////////////
int topro_prog_gain(struct usb_device *udev, unsigned char *pgain)
{
	int i;
	unsigned char *lisGain= pgain;
	printk("BulkGain1:"); for (i=0; i<9; i++) printk(" %02x", *lisGain++); printk("\n");
	printk("BulkGain2:"); for (i=0; i<9; i++) printk(" %02x", *lisGain++); printk("\n");

	return topro_bulk_out(udev, 0x03, pgain, GAIN_TABLE_LENGTH);
}

////////////////////////////////////////////////////////////////////////////////
// matrix3 = matrix1 * matrix2

////////////////////////////////////////////////////////////////////////////////


void matrix_3x3_mul(long matrix1[3][3], long matrix2[3][3], long matrix3[3][3])
{


	int i, j; // loop index





	for (i=0; i<3; i++) {
		for (j=0; j<3; j++) {
			matrix3[i][j] = (matrix1[i][0] * matrix2[0][j]) +



			(matrix1[i][1] * matrix2[1][j]) +
			(matrix1[i][2] * matrix2[2][j]);
		} // for
	} // for
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void matrix_div_round(long *pmatrix, long count, long unit)
{

	long index; // loop index


	for (index=0; index<count; index++)

	{

		if (*(pmatrix + index) >= 0)


			*(pmatrix + index) = (*(pmatrix + index) + (unit / 2)) / unit;

		else


			*(pmatrix + index) = (*(pmatrix + index) - (unit / 2)) / unit;
	} // for

}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//void matrix_3x3_print(long *pmatrix)
//{
//	printk("\n Matrix[3][3] = {\n");
//	printk("{%d, %d, %d},\n", (int)pmatrix[0], (int)pmatrix[1], (int)pmatrix[2]);
//	printk("{%d, %d, %d},\n", (int)pmatrix[3], (int)pmatrix[4], (int)pmatrix[5]);


//	printk("{%d, %d, %d}", (int)pmatrix[6], (int)pmatrix[7], (int)pmatrix[8]);

//	printk("};\n");

//}



#define AWB_ADJ_THD 2

////////////////////////////////////////////////////////////////////////////////
// Set hue/saturation function
////////////////////////////////////////////////////////////////////////////////

#if 1
//;
//; Commented by Joseph, Since method of topro_set_hue_saturation() conflict to topro_prog_gain(new_gain)
//; Date: 20061020
void topro_set_hue_saturation(struct usb_device *udev, long hue, long saturation)

{
	long RGB2YUV[3][3] = { // x 1000



		{ 299,  587,  114},
		{-169, -331,  500},

		{ 500, -418,  -81}};

	long HSB2RGB[3][3] = { // x 100

		{ -98,   38,  341},
		{  24, -183,  231},
		{ -24, -445,  265}};



	long IHSB2RGB[3][3] = { // x 10000
		{-4976,  14827, -6522},
		{ 1091,   1630, -2824},
		{ 1381,   4080, -1560}};

	long XYZ2RGB[3][3] = { // x 10000
		{ 32410, -15374, -4986},
		{ -9692,  18760,   416},
		{   556,  -2040, 10570}};

//	long AWB[3][3] =
//		{  4065,   6183,   -42},



//		{ -1991,  -4631,  5990},

//		{ 20211, -15363, -3523};

#ifdef ISSC
	long CCM[3][3] = { // TAS

		{ 5018,    2063,  2329},

		{  1492,  8102,  405},

		{-2037, -2766, 15695}};
#else    

	long CCM[3][3] = { // TAS

		{ 5913,    57,  5560},

		{  629,  8752,  2527},

		{-4852, -8344, 26757}};
#endif

//	long CCM[3][3] = { // EVS 350


//		{ 4764,  2574,  2164},
//		{ 1695,  7792,   513},
//		{-1346, -1417, 13637}};


	long HUE_SAT_ADJ[3][3] = {
		{0, 0, 0},


		{0, 0, 0},


		{0, 0, 0}};

	long HUE_SAT[3][3];

	long Gain[3][3];

	long matrixbff1[3][3];
	long matrixbff2[3][3];

	int i, j; // loop index
	short gainbulk[9];

	// intialize matrix
 
	HUE_SAT_ADJ[0][0] = hue * 2;
	HUE_SAT_ADJ[1][1] = saturation * 2;
	HUE_SAT_ADJ[2][2] = 100;

	matrix_3x3_mul(HSB2RGB, HUE_SAT_ADJ, matrixbff1);// x 100 x 10000
	matrix_div_round((long *)matrixbff1, 9, 100); // div 100
//	matrix_3x3_print((long *)matrixbff1);

	matrix_3x3_mul(matrixbff1, IHSB2RGB, HUE_SAT);// 10000, 10000
	matrix_div_round((long *)HUE_SAT, 9, 1000); // div 1000
//	matrix_3x3_print((long *)HUE_SAT);

	matrix_3x3_mul(RGB2YUV, HUE_SAT, matrixbff1); // 1000, 100000
	matrix_div_round((long *)matrixbff1, 9, 1000); // div 1000
//	matrix_3x3_print((long *)matrixbff1);

	matrix_3x3_mul(matrixbff1, XYZ2RGB, matrixbff2); // 100000, 10000
	matrix_div_round((long *)matrixbff2, 9, 10000); // div 10000
//	matrix_3x3_print((long *)matrixbff2);

	matrix_3x3_mul(matrixbff2, CCM, Gain); // 10000, 10000
//	matrix_div_round((long *)Gain, 9, 1000000);
//	matrix_3x3_print((long *)Gain);


//	printk("\nGain Bulk = {\n");
	for (i=0; i<3; i++)
	{
		for (j=0; j<3; j++)

			gainbulk[(i * 3) + j] = (short)maximum(minimum(((Gain[i][j] * 4)
						/ 156250), 511), -512);

//	printk("{%d, %d, %d},\n", gainbulk[(i * 3) + 0], gainbulk[(i * 3) + 1], gainbulk[(i * 3) + 2]);
	} // for


//	printk("};\n");




  if(AWB_FACTOR[0] > 0)
    gainbulk[6] = minimum(511,gainbulk[6]+AWB_FACTOR[0]);
  else
    gainbulk[6] = minimum(511,gainbulk[6]-(-AWB_FACTOR[0]));

  if(AWB_FACTOR[2] > 0)
    gainbulk[5] = minimum(511,gainbulk[5]+AWB_FACTOR[2]);
  else
    gainbulk[5] = minimum(511,gainbulk[5]-(-AWB_FACTOR[2]));    
        

//  printk("AWB Factor %d %d %d\n",AWB_FACTOR[0], AWB_FACTOR[1], AWB_FACTOR[2]);
//  printk("gainbulk %d %d %d %d %d\n",gainbulk[0], gainbulk[1],gainbulk[2], gainbulk[3],gainbulk[4]);
//  printk("gainbulk %d %d %d %d\n",gainbulk[5], gainbulk[6],gainbulk[7], gainbulk[8]);


	topro_prog_gain(udev, (unsigned char *)gainbulk);
}
#endif

// 2006.11.29 @ Sercomm/NanKon
void topro_setiris(struct usb_device *udev, int Exposure)
{
	unsigned char ucdat;

	printk("[%d] Prev:%d, ToBe:%d \n", ++sysIrisCount, IrisPrevIris, Exposure);

	if (Exposure < IrisPrevIris /*IrisPrevExposure*/)
	{
		//;	topro_write_spi16(udev, DAC_CONTROL_LSB, 0x00);
		//;	topro_write_spi16(udev, DAC_CONTROL_MSB, 0x00);

		ucdat= Get_TableItem(IrisPrevIris /*IrisPrevExposure*/, IRIS_CLOSE);
		ucdat <<= 2;
		topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat);

		printk(" (%d)", IrisPrevIris /*IrisPrevExposure*/);
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat >> 2);

		ucdat= 1;
		ucdat <<= 2;
		topro_write_spi16(udev, DAC_CONTROL_MSB, ucdat);  // 0x01 << 2

		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, 0x01);
		printk("\n");

		udelay(1);
		IrisPrevIris--;

		if (Exposure!=IrisPrevIris)
		{
			ucdat= Get_TableItem(Exposure+1, IRIS_CLOSE);
			ucdat <<= 2;
			topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat);

			printk(" (%d)", Exposure+1);
			printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat >> 2);

			ucdat= 1;
			ucdat <<= 2;
			topro_write_spi16(udev, DAC_CONTROL_MSB, ucdat);  // 0x01 << 2

			printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, 0x01);
			printk("\n");
		}
	
		udelay(1);

		ucdat= Get_TableItem(Exposure, IRIS_CLOSE);
		ucdat <<= 2;

		topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat);
		printk(" (%d)", Exposure);
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat >> 2);

		/* if (Exposure==1) {
		topro_write_spi16(udev, DAC_CONTROL_MSB, 0x02);
		printk(".Set %02x, %02x;  ", DAC_CONTROL_MSB, 0x02);
		} else */

		ucdat= 1;
		ucdat <<= 2;
		topro_write_spi16(udev, DAC_CONTROL_MSB, ucdat);  // 0x01 << 2
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, 0x01);
		printk("\n");
	}
	else if (Exposure > IrisPrevIris /*IrisPrevExposure*/)
	{

		ucdat= Get_TableItem(IrisPrevIris /*IrisPrevExposure*/, IRIS_OPEN);

		topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat << 2);
		printk(" (%d)", IrisPrevIris /*IrisPrevExposure*/);
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat);
		topro_write_spi16(udev, DAC_CONTROL_MSB, 0x01 << 2);
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, 0x01);
		printk("\n");

		udelay(1);
		IrisPrevIris++;
		if (Exposure!=IrisPrevIris)
		{
			ucdat= Get_TableItem(Exposure-1, IRIS_OPEN);

			topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat << 2);
			printk(" (%d)", Exposure-1);
			printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat);
			topro_write_spi16(udev, DAC_CONTROL_MSB, 0x01 << 2);
			printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, 0x01);
			printk("\n");
		}

		udelay(1);
		ucdat= Get_TableItem(Exposure, IRIS_OPEN);

		topro_write_spi16(udev, DAC_CONTROL_LSB, ucdat << 2);
		printk(" (%d)", Exposure);
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_LSB, ucdat);
		topro_write_spi16(udev, DAC_CONTROL_MSB, 0x01 << 2);
		printk(".Set %02xh, %02d(<<2);  ", DAC_CONTROL_MSB, 0x01);
		printk("\n");
	}
	else{
		printk(".Set Exposure take no effect! (Eq. previous one) <<Init>>\n");
	}

	IrisPrevIris /*IrisPrevExposure*/= Exposure;
}

void topro_setexposure(struct usb_device *udev, int Exposure,struct pwc_device *pdev )
//;"TP6830_MT9M001"
//;"TP6830_MT9V011"
//;"TP6830_SHARPCCD"
//;"TP6830_TICCD"
//;"TP6830_SONYCCD"
{
  unsigned char temp1, temp2, temp3;
	long temp4;
  int i;
  int mclk;

//	static int ImageSize=VGA;
	
						unsigned char MT9V011_Exposure[300] = {
									0x01, 0x10, 0xd1, 0x01, 0x14, 0xd1, 0x01, 0x18, 0xd1, 0x01, 0x1c, 0xd1, 0x01, 0x20, 0xd1,

									0x01, 0x24, 0xd1, 0x01, 0x28, 0xd1, 0x01, 0x2c, 0xd1, 0x01, 0x30, 0xd1, 0x01, 0x34, 0xd1,
									0x01, 0x38, 0xd1, 0x01, 0x3c, 0xd1, 0x01, 0xa0, 0xd1, 0x01, 0xa2, 0xd1, 0x01, 0xa4, 0xd1,

									0x01, 0xa6, 0xd1, 0x01, 0xa8, 0xd1, 0x01, 0xaa, 0xd1, 0x01, 0xac, 0xd1, 0x01, 0xae, 0xd1,


									0x01, 0xb0, 0xd1, 0x02, 0x30, 0xd1, 0x02, 0x34, 0xd1, 0x02, 0x38, 0xd1, 0x02, 0x3c, 0xd1,
									0x02, 0xa0, 0xd1, 0x02, 0xa2, 0xd1, 0x02, 0xa4, 0xd1, 0x02, 0xa6, 0xd1, 0x02, 0xa8, 0xd1,


									0x02, 0xaa, 0xd1, 0x02, 0xac, 0xd1, 0x02, 0xae, 0xd1, 0x02, 0xb0, 0xd1, 0x03, 0x3f, 0xd1,
									0x03, 0xa0, 0xd1, 0x03, 0xa2, 0xd1, 0x03, 0xa4, 0xd1, 0x03, 0xa6, 0xd1, 0x03, 0xa8, 0xd1,

									0x03, 0xaa, 0xd1, 0x03, 0xac, 0xd1, 0x03, 0xae, 0xd1, 0x03, 0xb0, 0xd1, 0x04, 0xa4, 0xd1,




									0x04, 0xa6, 0xd1, 0x04, 0xa8, 0xd1, 0x04, 0xaa, 0xd1, 0x04, 0xac, 0xd1, 0x04, 0xae, 0xd1,
									0x04, 0xb0, 0xd1, 0x05, 0xa6, 0xd1, 0x05, 0xa8, 0xd1, 0x05, 0xaa, 0xd1, 0x05, 0xac, 0xd1,
									0x05, 0xae, 0xd1, 0x05, 0xb0, 0xd1, 0x06, 0xa8, 0xd1, 0x06, 0xaa, 0xd1, 0x06, 0xac, 0xd1,
									0x06, 0xae, 0xd1, 0x06, 0xb0, 0xd1, 0x07, 0xaa, 0xd1, 0x07, 0xac, 0xd1, 0x07, 0xae, 0xd1,
									0x07, 0xb0, 0xd1, 0x08, 0xac, 0xd1, 0x08, 0xae, 0xd1, 0x08, 0xb0, 0xd1, 0x09, 0xac, 0xd1,
									0x09, 0xae, 0xd1, 0x09, 0xb0, 0xd1, 0x0a, 0xac, 0xd1, 0x0a, 0xae, 0xd1, 0x0a, 0xb0, 0xd1,
									0x0b, 0xb0, 0xd1, 0x0b, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0d, 0xb0, 0xd1,
									0x0d, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1,
									0x10, 0xb0, 0xd1, 0x10, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x12, 0xb0, 0xd1,
									0x12, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x14, 0xb0, 0xd1, 0x14, 0xb0, 0xd1,


									0x15, 0xb0, 0xd1, 0x16, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1,



						};

					  unsigned char MT9V011_Exposure60[300] = {


									0x01, 0x10, 0xd1, 0x01, 0x14, 0xd1, 0x01, 0x18, 0xd1, 0x01, 0x1c, 0xd1, 0x01, 0x20, 0xd1,
									0x01, 0x24, 0xd1, 0x01, 0x28, 0xd1, 0x01, 0x2c, 0xd1, 0x01, 0x30, 0xd1, 0x01, 0x34, 0xd1,
									0x01, 0x38, 0xd1, 0x01, 0x3c, 0xd1, 0x01, 0xa0, 0xd1, 0x01, 0xa2, 0xd1, 0x01, 0xa4, 0xd1,
									0x01, 0xa6, 0xd1, 0x01, 0xa8, 0xd1, 0x01, 0xaa, 0xd1, 0x01, 0xac, 0xd1, 0x01, 0xae, 0xd1,
									0x01, 0xb0, 0xd1, 0x02, 0x30, 0xd1, 0x02, 0x34, 0xd1, 0x02, 0x38, 0xd1, 0x02, 0x3c, 0xd1,
									0x02, 0xa0, 0xd1, 0x02, 0xa2, 0xd1, 0x02, 0xa4, 0xd1, 0x02, 0xa6, 0xd1, 0x02, 0xa8, 0xd1,
									0x02, 0xaa, 0xd1, 0x02, 0xac, 0xd1, 0x02, 0xae, 0xd1, 0x02, 0xb0, 0xd1, 0x03, 0x3f, 0xd1,
									0x03, 0xa0, 0xd1, 0x03, 0xa2, 0xd1, 0x03, 0xa4, 0xd1, 0x03, 0xa6, 0xd1, 0x03, 0xa8, 0xd1,
									0x03, 0xaa, 0xd1, 0x03, 0xac, 0xd1, 0x03, 0xae, 0xd1, 0x03, 0xb0, 0xd1, 0x04, 0xa4, 0xd1,


									0x04, 0xa6, 0xd1, 0x04, 0xa8, 0xd1, 0x04, 0xaa, 0xd1, 0x04, 0xac, 0xd1, 0x04, 0xae, 0xd1,
									0x04, 0xb0, 0xd1, 0x05, 0xa6, 0xd1, 0x05, 0xa8, 0xd1, 0x05, 0xaa, 0xd1, 0x05, 0xac, 0xd1,

									0x05, 0xae, 0xd1, 0x05, 0xb0, 0xd1, 0x06, 0xa8, 0xd1, 0x06, 0xaa, 0xd1, 0x06, 0xac, 0xd1,
									0x06, 0xae, 0xd1, 0x06, 0xb0, 0xd1, 0x07, 0xaa, 0xd1, 0x07, 0xac, 0xd1, 0x07, 0xae, 0xd1,
									0x07, 0xb0, 0xd1, 0x08, 0xac, 0xd1, 0x08, 0xae, 0xd1, 0x08, 0xb0, 0xd1, 0x09, 0xac, 0xd1,
									0x09, 0xae, 0xd1, 0x09, 0xb0, 0xd1, 0x0a, 0xac, 0xd1, 0x0a, 0xae, 0xd1, 0x0a, 0xb0, 0xd1,
									0x0b, 0xb0, 0xd1, 0x0b, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0d, 0xb0, 0xd1,
									0x0d, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1,
									0x10, 0xb0, 0xd1, 0x10, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x12, 0xb0, 0xd1,
									0x12, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x14, 0xb0, 0xd1, 0x14, 0xb0, 0xd1,

									0x15, 0xb0, 0xd1, 0x16, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1,
						};
						unsigned char MT9V011_CIF_Exposure[300] = { 
									0x01, 0x10, 0xd1, 0x01, 0x14, 0xd1, 0x01, 0x18, 0xd1, 0x01, 0x1c, 0xd1, 0x01, 0x20, 0xd1,
									0x01, 0x24, 0xd1, 0x01, 0x28, 0xd1, 0x01, 0x2c, 0xd1, 0x01, 0x30, 0xd1, 0x01, 0x34, 0xd1,




									0x01, 0x38, 0xd1, 0x01, 0x3c, 0xd1, 0x01, 0xa0, 0xd1, 0x01, 0xa2, 0xd1, 0x01, 0xa4, 0xd1,
									0x01, 0xa6, 0xd1, 0x01, 0xa8, 0xd1, 0x01, 0xaa, 0xd1, 0x01, 0xac, 0xd1, 0x01, 0xae, 0xd1,
									0x01, 0xb0, 0xd1, 0x02, 0x30, 0xd1, 0x02, 0x34, 0xd1, 0x02, 0x38, 0xd1, 0x02, 0x3c, 0xd1,
									0x02, 0xa0, 0xd1, 0x02, 0xa2, 0xd1, 0x02, 0xa4, 0xd1, 0x02, 0xa6, 0xd1, 0x02, 0xa8, 0xd1,

									0x02, 0xaa, 0xd1, 0x02, 0xac, 0xd1, 0x02, 0xae, 0xd1, 0x02, 0xb0, 0xd1, 0x03, 0x3f, 0xd1,

									0x03, 0xa0, 0xd1, 0x03, 0xa2, 0xd1, 0x03, 0xa4, 0xd1, 0x03, 0xa6, 0xd1, 0x03, 0xa8, 0xd1,
									0x03, 0xaa, 0xd1, 0x03, 0xac, 0xd1, 0x03, 0xae, 0xd1, 0x03, 0xb0, 0xd1, 0x03, 0xb0, 0xd2, 
									0x03, 0xb0, 0xd3, 0x03, 0xb0, 0xd4, 0x03, 0xb0, 0xd5, 0x03, 0xb0, 0xd6, 0x04, 0xb0, 0xd1,
									0x04, 0xb0, 0xd2, 0x04, 0xb0, 0xd3, 0x04, 0xb0, 0xd4, 0x05, 0xb0, 0xd1, 0x05, 0xb0, 0xd2,
									0x05, 0xb0, 0xd3, 0x05, 0xb0, 0xd4, 0x05, 0xb0, 0xd5, 0x05, 0xb0, 0xd6, 0x05, 0xb0, 0xd7,


									0x06, 0xb0, 0xd4, 0x06, 0xb0, 0xd5, 0x06, 0xb0, 0xd6, 0x07, 0xb0, 0xd3, 0x07, 0xb0, 0xd3,

									0x08, 0xb0, 0xd1, 0x08, 0xb0, 0xd1, 0x08, 0xb0, 0xd2, 0x08, 0xb0, 0xd2, 0x09, 0xae, 0xd1,
									0x09, 0xae, 0xd1, 0x09, 0xb0, 0xd1, 0x0a, 0xac, 0xd1, 0x0a, 0xae, 0xd1, 0x0a, 0xb0, 0xd1,

									0x0b, 0xb0, 0xd1, 0x0b, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0d, 0xb0, 0xd1,
									0x0d, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1,








									0x10, 0xb0, 0xd1, 0x10, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x12, 0xb0, 0xd1,


									0x12, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x14, 0xb0, 0xd1, 0x14, 0xb0, 0xd1,
									0x15, 0xb0, 0xd1, 0x16, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1,
						};

						unsigned char MT9V011_CIF_Exposure60[300] = {

									0x01, 0x10, 0xd1, 0x01, 0x14, 0xd1, 0x01, 0x18, 0xd1, 0x01, 0x1c, 0xd1, 0x01, 0x20, 0xd1,
									0x01, 0x24, 0xd1, 0x01, 0x28, 0xd1, 0x01, 0x2c, 0xd1, 0x01, 0x30, 0xd1, 0x01, 0x34, 0xd1,






									0x01, 0x38, 0xd1, 0x01, 0x3c, 0xd1, 0x01, 0xa0, 0xd1, 0x01, 0xa2, 0xd1, 0x01, 0xa4, 0xd1,
									0x01, 0xa6, 0xd1, 0x01, 0xa8, 0xd1, 0x01, 0xaa, 0xd1, 0x01, 0xac, 0xd1, 0x01, 0xae, 0xd1,
									0x01, 0xb0, 0xd1, 0x02, 0x30, 0xd1, 0x02, 0x34, 0xd1, 0x02, 0x38, 0xd1, 0x02, 0x3c, 0xd1,
									0x02, 0xa0, 0xd1, 0x02, 0xa2, 0xd1, 0x02, 0xa4, 0xd1, 0x02, 0xa6, 0xd1, 0x02, 0xa8, 0xd1,

									0x02, 0xaa, 0xd1, 0x02, 0xac, 0xd1, 0x02, 0xae, 0xd1, 0x02, 0xb0, 0xd1, 0x03, 0x3f, 0xd1,
									0x03, 0xa0, 0xd1, 0x03, 0xa2, 0xd1, 0x03, 0xa4, 0xd1, 0x03, 0xa6, 0xd1, 0x03, 0xa8, 0xd1,

									0x03, 0xaa, 0xd1, 0x03, 0xac, 0xd1, 0x03, 0xae, 0xd1, 0x03, 0xb0, 0xd1, 0x03, 0xb0, 0xd2, 
									0x03, 0xb0, 0xd3, 0x03, 0xb0, 0xd4, 0x03, 0xb0, 0xd5, 0x03, 0xb0, 0xd6, 0x04, 0xb0, 0xd1,
									0x04, 0xb0, 0xd2, 0x04, 0xb0, 0xd3, 0x04, 0xb0, 0xd4, 0x05, 0xb0, 0xd1, 0x05, 0xb0, 0xd2,




									0x05, 0xb0, 0xd3, 0x05, 0xb0, 0xd4, 0x05, 0xb0, 0xd5, 0x05, 0xb0, 0xd6, 0x05, 0xb0, 0xd7,
									0x06, 0xb0, 0xd4, 0x06, 0xb0, 0xd5, 0x06, 0xb0, 0xd6, 0x07, 0xb0, 0xd3, 0x07, 0xb0, 0xd3,
									0x08, 0xb0, 0xd1, 0x08, 0xb0, 0xd1, 0x08, 0xb0, 0xd2, 0x08, 0xb0, 0xd2, 0x09, 0xae, 0xd1,
									0x09, 0xae, 0xd1, 0x09, 0xb0, 0xd1, 0x0a, 0xac, 0xd1, 0x0a, 0xae, 0xd1, 0x0a, 0xb0, 0xd1,

									0x0b, 0xb0, 0xd1, 0x0b, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0c, 0xb0, 0xd1, 0x0d, 0xb0, 0xd1,

									0x0d, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0e, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1, 0x0f, 0xb0, 0xd1,
									0x10, 0xb0, 0xd1, 0x10, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x11, 0xb0, 0xd1, 0x12, 0xb0, 0xd1,
									0x12, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x13, 0xb0, 0xd1, 0x14, 0xb0, 0xd1, 0x14, 0xb0, 0xd1,


									0x15, 0xb0, 0xd1, 0x16, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1, 0x17, 0xb0, 0xd1,
						};

#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 [(set exposure)]

					  MaxExposure = 99;
						
					if (Exposure < 0)
						Exposure = 0;
					if (Frequency & (Exposure < 8))
						Exposure = 8;
					if (Exposure > MaxExposure)
						Exposure = MaxExposure;

					
					//	if (BitmapWidth == 640){
							if (FrameRate == 30){
								i = 2;
						  mclk = 24;

						}
							else if (FrameRate == 20){
								i = 3;
						  mclk = 24;
						}
							else if (FrameRate == 15){

								i = 4;
						  mclk = 24;
						}
							else if (FrameRate == 10){
								i = 6;
						  mclk = 24;

						}
							else if (FrameRate == 5){
								i = 12;
						  mclk = 24;
						}
							else if (FrameRate == 3){
								i = 20;
						  mclk = 24;
						}
							else{ //FrameRate = 1
								i = 20;
						  mclk = 8;
						}
					//	}

						//Begin
						//;not; topro_write_i2c(udev, 0x5d, 0x07, 0x00, 0x03);
							
						if (Frequency){
						  if (BitmapWidth == 640){

								if (Frequency == 60){
									temp1 = MT9V011_Exposure60[Exposure*3+1]; //;MT9M001_Exposure60
									temp2 = MT9V011_Exposure60[Exposure*3]; //;MT9M001_Exposure60
									temp3 = MT9V011_Exposure60[Exposure*3+2]; //;MT9M001_Exposure60
								}
								else{
									temp1 = MT9V011_Exposure[Exposure*3+1]; //;MT9M001_Exposure
									temp2 = MT9V011_Exposure[Exposure*3]; //;MT9M001_Exposure
									temp3 = MT9V011_Exposure[Exposure*3+2]; //;MT9M001_Exposure
								}
							}
							else{  //QVGA
								if (Frequency == 60){
									temp1 = MT9V011_CIF_Exposure60[Exposure*3+1]; //;MT9M001_CIF_Exposure60
									temp2 = MT9V011_CIF_Exposure60[Exposure*3]; //;MT9M001_CIF_Exposure60
									temp3 = MT9V011_CIF_Exposure60[Exposure*3+2]; //;MT9M001_CIF_Exposure60
								}
								else{
									temp1 = MT9V011_CIF_Exposure[Exposure*3+1]; //;MT9M001_CIF_Exposure
									temp2 = MT9V011_CIF_Exposure[Exposure*3]; //;MT9M001_CIF_Exposure
									temp3 = MT9V011_CIF_Exposure[Exposure*3+2]; //;MT9M001_CIF_Exposure
								}
							}

						  //=WriteI2C_Word(DC, 0x5d, 0x35, 0x00, temp1);
							topro_write_i2c(udev, 0x5d, 0x35, 0x00, temp1);
							  printk("Exposure %d, [0x%02x, 0x%02x, 0x%02x]\n", Exposure, 0x35, 0x00, temp1); // +
							
							Exposure= temp2;
							      // ;not; temp4 = (Exposure * mclk * 1000000 / (Frequency * 2) + 228) / (801*i);
							temp4 = (Exposure * 24000000 / (Frequency * 2) + 224) / (792*i);
							temp1 = temp4 & 0xff;
							temp2 = temp4 >> 8;

							topro_write_i2c(udev, 0x5d, 0x09, temp2, temp1);
							  printk("Exposure %d, [0x%02x, 0x%02x, 0x%02x]\n", Exposure, 0x35, 0x09, temp1); // +

						  if (BitmapWidth != 640)
							{
								topro_write_i2c(udev, 0x5d, 0x41, 0x00, temp3);
							  printk("Not-640 Exposure %d, [0x%02x, 0x%02x, 0x%02x]\n", Exposure, 0x41, 0x00, temp3); // +
							}

						}  //Frequency
						else{
							topro_write_i2c(udev, 0x5d, 0x35, 0x00, 0x30);

							temp4 = Exposure;
							temp1 = temp4 & 0xff;
							temp2 = temp4 >> 8;
							topro_write_i2c(udev, 0x5d, 0x09, temp2, temp1);
							topro_write_i2c(udev, 0x5d, 0x41, 0x00, 0xd1);
						}

						//End
						topro_write_i2c(udev, 0x5d, 0x07, 0x00, 0x02);
							  printk("Exposure %d, [0x%02x, 0x%02x, 0x%02x]\n", Exposure, 0x07, 0x00, 0x02);  // +

#elif defined(TP6830_MT9V011)

					  static int SensorVersion=0x8221;

					  MaxExposure = 99;

					  if (Exposure < 0)
						  Exposure = 0;
      
					  if (Exposure > MaxExposure)
						  Exposure = MaxExposure;


					//	topro_write_i2c(udev, 0x5d, 0x07, 0x00, 0x03);
					//	topro_write_i2c(udev, 0x5d, 0x0a, 0x00, 0x01);

					//	if (BitmapWidth == 640){
							if (FrameRate == 30){
								i = 2;
						  mclk = 24;

						}
							else if (FrameRate == 20){
								i = 3;
						  mclk = 24;
						}
							else if (FrameRate == 15){

								i = 4;
						  mclk = 24;
						}
							else if (FrameRate == 10){
								i = 6;
						  mclk = 24;

						}
							else if (FrameRate == 5){
								i = 12;
						  mclk = 24;
						}
							else if (FrameRate == 3){
								i = 20;
						  mclk = 24;
						}
							else{ //FrameRate = 1
								i = 20;
						  mclk = 8;
						}
					//	}
					//	else
					//		i = 2;

						topro_write_i2c(udev, 0x5d, 0x07, 0x00, 0x03);
  
						if (Frequency){

							topro_write_i2c(udev, 0x5d, 0x0a, 0x00, i-2);

						  if (BitmapWidth == 640){

								if (Frequency == 60){
									temp1 = MT9V011_Exposure60[Exposure*3+1];
									temp2 = MT9V011_Exposure60[Exposure*3];
									temp3 = MT9V011_Exposure60[Exposure*3+2];
								}
								else{
									temp1 = MT9V011_Exposure[Exposure*3+1];
									temp2 = MT9V011_Exposure[Exposure*3];
									temp3 = MT9V011_Exposure[Exposure*3+2];
								}
							}
							else{  //QVGA
								if (Frequency == 60){
									temp1 = MT9V011_CIF_Exposure60[Exposure*3+1];
									temp2 = MT9V011_CIF_Exposure60[Exposure*3];
									temp3 = MT9V011_CIF_Exposure60[Exposure*3+2];
								}
								else{
									temp1 = MT9V011_CIF_Exposure[Exposure*3+1];
									temp2 = MT9V011_CIF_Exposure[Exposure*3];
									temp3 = MT9V011_CIF_Exposure[Exposure*3+2];
								}
							}

						  topro_write_i2c(udev, 0x5d, 0x35, 0x00, temp1);
						  Exposure = temp2;

					//		if (SensorVersion == 0x8221)
					//			temp4 = (Exposure * 24000000 / (Frequency * 2) + 224) / (792*i);
					//		else
					//			temp4 = (Exposure * 24000000 / (Frequency * 2) + 224) / (786*i);
							temp4 = (Exposure * mclk * 1000000 / (Frequency * 2) + 228) / (801*i);
							temp1 = temp4 & 0xff;
							temp2 = temp4 >> 8;
							topro_write_i2c(udev, 0x5d, 0x09, temp2, temp1);

						  if (BitmapWidth != 640)
								topro_write_i2c(udev, 0x5d, 0x41, 0x00, temp3);
      
						}  //Frequency
						else{
							topro_write_i2c(udev, 0x5d, 0x35, 0x00, 0x30);

							temp4 = Exposure;
							temp1 = temp4 & 0xff;
							temp2 = temp4 >> 8;
							topro_write_i2c(udev, 0x5d, 0x09, temp2, temp1);
							topro_write_i2c(udev, 0x5d, 0x41, 0x00, 0xd1);
						}

						topro_write_i2c(udev, 0x5d, 0x07, 0x00, 0x02);

/////////////
//TP6812 SharpCCD
/////////////////  
#elif defined(TP6830_SHARPCCD)

						unsigned char SHARPCCD_Exposure60[416]={

						//
						  0x01,0x01,0xb0,0x00,0x00, 0x01,0x01,0xe0,0x00,0x00, 0x01,0x01,0xff,0x00,0x00,
						  0x02,0x01,0xb0,0x00,0x00, 0x02,0x01,0xe0,0x00,0x00, 0x02,0x01,0xff,0x00,0x00,	//;1~4	L1~L12
   						  0x03,0x01,0xb0,0x00,0x00, 0x03,0x01,0xe0,0x00,0x00, 0x03,0x01,0xff,0x00,0x00,
						  0x04,0x01,0xb0,0x00,0x00, 0x04,0x01,0xe0,0x00,0x00, 0x04,0x01,0xff,0x00,0x00,

						  0x05,0x01,0xc0,0x00,0x00, 0x05,0x01,0xe0,0x00,0x00, 0x05,0x01,0xff,0x00,0x00,
						  0x06,0x01,0xd0,0x00,0x00, 0x06,0x01,0xe0,0x00,0x00, 0x06,0x01,0xff,0x00,0x00,	//;5,6	L13~L18
						  0x07,0x01,0xd8,0x00,0x00, 0x07,0x01,0xe8,0x00,0x00, 0x07,0x01,0xff,0x00,0x00,
						  0x08,0x01,0xd8,0x00,0x00, 0x08,0x01,0xe8,0x00,0x00, 0x08,0x01,0xff,0x00,0x00,	//;7,8	L19~L24
   						  0x09,0x01,0xff,0x00,0x00,

						  0x0a,0x01,0xff,0x00,0x00,

						  0x0b,0x01,0xff,0x00,0x00,

						  0x0c,0x01,0xff,0x00,0x00,						//;9~12	L25~L28
						  0x0d,0x01,0xff,0x00,0x00,
						  0x0e,0x01,0xff,0x00,0x00,
						  0x0f,0x01,0xff,0x00,0x00,
						  0x10,0x01,0xff,0x00,0x00,						//;13~16	L29~L32
   						  0x11,0x01,0xff,0x00,0x00,
						  0x12,0x01,0xff,0x00,0x00,
						  0x13,0x01,0xff,0x00,0x00,
						  0x14,0x01,0xff,0x00,0x00,						//;17~24	L33~L52
						  0x15,0x01,0xff,0x00,0x00,
						  0x16,0x01,0xff,0x00,0x00,
						  0x17,0x01,0xff,0x00,0x00,
						  0x18,0x01,0xff,0x00,0x00,

						  0x18,0x01,0x40,0x01,0x00,

						  0x18,0x01,0x80,0x01,0x00,    };
						unsigned char SHARPCCD_Exposure50[416]={
						//
						  0x01,0x01,0xb0,0x00,0x00, 0x01,0x01,0xe0,0x00,0x00, 0x01,0x01,0xff,0x00,0x00,
						  0x02,0x01,0xb0,0x00,0x00, 0x02,0x01,0xe0,0x00,0x00, 0x02,0x01,0xff,0x00,0x00,	//;1~4	L1~L12
   						  0x03,0x01,0xb0,0x00,0x00, 0x03,0x01,0xe0,0x00,0x00, 0x03,0x01,0xff,0x00,0x00,
						  0x04,0x01,0xc0,0x00,0x00, 0x04,0x01,0xd0,0x00,0x00, 0x04,0x01,0xe0,0x00,0x00,
 						  0x05,0x01,0xb0,0x00,0x00, 0x05,0x01,0xc0,0x00,0x00, 0x05,0x01,0xd0,0x00,0x00, 0x05,0x01,0xe0,0x00,0x00,



						  0x06,0x01,0xd0,0x00,0x00, 0x06,0x01,0xe0,0x00,0x00,	//;5,6	L13~L18
						  0x07,0x01,0xd8,0x00,0x00, 0x07,0x01,0xe8,0x00,0x00, 0x07,0x01,0xff,0x00,0x00,
						  0x08,0x01,0xd8,0x00,0x00, 0x08,0x01,0xe8,0x00,0x00, 0x08,0x01,0xff,0x00,0x00,	//;7,8	L19~L24

						  0x09,0x01,0xff,0x00,0x00,
						  0x0a,0x01,0xff,0x00,0x00,

						  0x0b,0x01,0xff,0x00,0x00,
						  0x0c,0x01,0xff,0x00,0x00,						//;9~12	L25~L28


						  0x0d,0x01,0xff,0x00,0x00,
						  0x0e,0x01,0xff,0x00,0x00,

						  0x0f,0x01,0xff,0x00,0x00,
						  0x10,0x01,0xff,0x00,0x00,						//;13~16	L29~L32


						  0x11,0x01,0xff,0x00,0x00,





						  0x12,0x01,0xff,0x00,0x00,
						  0x13,0x01,0xff,0x00,0x00,
						  0x14,0x01,0xff,0x00,0x00,						//;17~24	L33~L52
						  0x14,0x01,0xff,0x00,0x00,
						  0x14,0x01,0xff,0x00,0x00,
						  0x14,0x01,0xff,0x00,0x00,

						  0x14,0x01,0xff,0x00,0x00,

						  0x14,0x01,0x40,0x01,0x00,

						  0x14,0x01,0x80,0x01,0x00,

						};

					  MaxExposure = 41;

					  if (Exposure < 0)
						  Exposure = 0;

					  if (Exposure > MaxExposure)
						  Exposure = MaxExposure;
      
						unsigned char *ccdTemp;

						unsigned char RegData,RegData2;
					  unsigned char CCDTemp1,CCDTemp2;
						long CCDExposure;



									if (Exposure < 1)
										Exposure = 1;


									if (Exposure > MaxExposure)
										Exposure = MaxExposure;

								printk("CCD Exposure %d\n", Exposure);
									if (Frequency == 50){


										if (Exposure <=8)        //30fps
											temp3 = 0x03;  //0x86
										else if (Exposure <=15)   //20fps
											temp3 = 0x05;
										else if (Exposure <=17)   //15fps


											temp3 = 0x07;


										else if (Exposure <=25)   //10 fps
											temp3 = 0x0b;
										else if (Exposure <=28)   //7.5 fps
											temp3 = 0x0f;
										else
											temp3 = 0x25;
									}

									else if (Frequency == 60){
										if (Exposure <=11)
											temp3 = 0x03;  //0x86
										else if (Exposure <=17)
											temp3 = 0x05;
										else if (Exposure <=23)
											temp3 = 0x07;

										else if (Exposure <=27)
											temp3 = 0x0b;
										else if (Exposure <=31)

											temp3 = 0x0f;

										else

											temp3 = 0x25;
									}
									else{
										temp3 = 0x03;
									}


							if(temp3 == 0x03)

										temp4 = 12;
									else if(temp3 == 0x05)
										temp4 = 8;

									else if(temp3 == 0x07)


										temp4 = 6;
									else if(temp3 == 0x0b)
										temp4 = 4;
									else if(temp3 == 0x0f)
										temp4 = 3;


									else if(temp3 == 0x25)
										temp4 = 2;


								if(Frequency == 50){
										CCDExposure = 0x20c - (SHARPCCD_Exposure50[Exposure*5] * temp4 * 1000000 / (Frequency * 2) - 447) / 780;

								}
								else if(Frequency == 60){

										CCDExposure = 0x20c - (SHARPCCD_Exposure60[Exposure*5] * temp4 * 1000000 / (Frequency * 2) - 447) / 780;

								}


						  else
										CCDExposure = 0x20c - Exposure * 4;

								CCDTemp1 = (CCDExposure << 4) & 0xf0;

								CCDTemp2 = (CCDExposure & 0xffff) >> 4;


									RegData = topro_read_reg(udev, MCLK_SEL);
									if(RegData != temp3){

										topro_write_reg(udev, MCLK_SEL, temp3);
										topro_write_reg(udev, MCLK_CFG, 0x03);
          
										topro_write_i2c_ccd(udev, 0x15, 0x01, 0x00, 0x00);
										for(i=0;i<100;i++){  //Waiting for Sif_Control be reseted;
											RegData2 = topro_read_reg(udev, SIF_CONTROL);
											if(RegData2 == 0x00){
												break;
											}
					//						Sleep(-10000*1);



										}

					//					DBGOUT5(("Wait FRAME_RATE %d ms",i));
									}


									if (Frequency){


											if (Exposure >= 33){
												topro_write_i2c_ccd(udev, 0x36, 0x56, 0x00, 0x00);
											}
											else{
												topro_write_i2c_ccd(udev, 0x36, 0x21, 0x00, 0x00);
											}

											for(i=0;i<100;i++){  //Waiting for Sif_Control be reseted;
												RegData2 = topro_read_reg(udev, SIF_CONTROL);

												if(RegData2 == 0x00){
													break;




												}



					//						DBGOUT5(("Wait SENSOR_CFG %d ms",i));
										}
									}
					// for reducing CCD noise in low light status

					//				DBGOUT5(("PreExposure Exposure %d %d",DC->PreExposure, DC->Exposure));




									PreExposure = Exposure;
									if(Frequency){
										if(Frequency == 50)
											ccdTemp = SHARPCCD_Exposure50 + Exposure*5;
										else
											ccdTemp = SHARPCCD_Exposure60 + Exposure*5;

							  topro_write_i2c(udev, 0x63,	0x01, CCDTemp1, CCDTemp2);
										topro_write_i2c(udev, *(ccdTemp+1), *(ccdTemp+2), *(ccdTemp+3), *(ccdTemp+4));
					//					DBGOUT5(("CCD Exposure %d Clk %x ", Exposure, temp3));
					//					DBGOUT5(("WriteI2C %x %x %x %x ", *ccdTemp,		*(ccdTemp+1), *(ccdTemp+2), *(ccdTemp+3)));
					//					DBGOUT5(("WriteI2C %x %x %x %x ",*(ccdTemp+4), *(ccdTemp+5), *(ccdTemp+6), *(ccdTemp+7)));
									}
							else{

							  topro_write_i2c(udev, 0x63,	0x01, CCDTemp1, CCDTemp2);
										topro_write_i2c(udev, 0x01, 0x40, 0x00, 0x00);
										topro_write_i2c(udev, 0x36, 0x21, 0x00, 0x00);
							}

#elif defined(TP6830_TICCD)

	//;unsigned char *ccdTemp;
	//;unsigned char RegData2;
		long CCDExposure;

#if 1
		Exposure= CheckRange_Exposure(1, MaxExposure, Exposure);
		CCDExposure= Calc_TempExposure(Exposure);

        if(Frequency)
				{
					long CCDGain;
					//;float digi_gain_10bit;
					int digi_gain_10bit, dig, flo, temp;

					unsigned char ucdat;
					unsigned char CCDTemp1 = (CCDExposure << 2) & 0xfc;				//low byte
					unsigned char CCDTemp2 = (CCDExposure >> 4) & 0xfc;				//high byte

		if(Exposure>=70)
		{
			  topro_prog_gamma(pdev, 0x00);   
			  topro_prog_gamma(pdev, 0x01);   
			  topro_prog_gamma(pdev, 0x02);   
			  //printk("=================low %d\n",Exposure);
		}
		else
		{
			  topro_prog_gamma1(pdev, 0x00);   
			  topro_prog_gamma1(pdev, 0x01);   
			  topro_prog_gamma1(pdev, 0x02);   
			  //printk("=================normal %d\n",Exposure);
		}

					topro_write_spi16(udev, 0x9e, CCDTemp1); WAIT_SIF(udev); //; SPI_16
					topro_write_spi16(udev, 0x9f, CCDTemp2); WAIT_SIF(udev); //; SPI_16

					CCDGain= Calc_CCDGain(Exposure);
					topro_write_spi16(udev, 0x07, CCDGain);  WAIT_SIF(udev); //; SPI_16

					//printk("0x9e: %x, 0x9f: %x\n", CCDTemp1, CCDTemp2);


//; fps control: <empty>

					ucdat= CCDGain >>2;

					digi_gain_10bit= ucdat;
			//; digi_gain_10bit <<= 6;
					digi_gain_10bit *= 64;

			//;	digi_gain_10bit= (digi_gain_10bit*0.03125)-6;
					
					digi_gain_10bit *= 3125;

					dig= digi_gain_10bit / 100000;
						flo= digi_gain_10bit / 1000;
						flo= flo % 100;

						temp= dig*100 + flo;

					if (dig>=6){

						dig -= 6;
						#ifdef _EXP_DBMSG_
						printk(".setexposure (%d):: H09e %03x, H007 (%02x, %d.%02d dB)", 
							Exposure, CCDExposure,ucdat, dig, flo);
						printk("\n");
						#endif
					}else{
						//;
						//; temp= digi*100 + flo;
						temp= 600 - temp;
						dig= temp / 100;
						flo= temp % 100;
						#ifdef _EXP_DBMSG_
						printk(".setexposure (%d):: H09e %03x, H007 (%02x, -%d.%02d dB) \n", 
												Exposure, CCDExposure,ucdat, dig, flo);
						#endif
					}

					AccYSmall= 256; 
					AccYLarge= 0;
				}
#endif
               
#elif defined(TP6830_SONYCCD)
	unsigned char SONYCCD_Exposure60[416]={
	// [0]Exposure time, [1~4]gain

	  0x01,0x01,0xb0,0x00,0x00, 0x01,0x01,0xe0,0x00,0x00, 0x01,0x01,0xff,0x00,0x00,
	  0x02,0x01,0xb0,0x00,0x00, 0x02,0x01,0xe0,0x00,0x00, 0x02,0x01,0xff,0x00,0x00,	//;1~4	L1~L12
   	  0x03,0x01,0xb0,0x00,0x00, 0x03,0x01,0xe0,0x00,0x00, 0x03,0x01,0xff,0x00,0x00,



	  0x04,0x01,0xb0,0x00,0x00, 0x04,0x01,0xe0,0x00,0x00, 0x04,0x01,0xff,0x00,0x00,
	  0x05,0x01,0xc0,0x00,0x00, 0x05,0x01,0xe0,0x00,0x00, 0x05,0x01,0xff,0x00,0x00,
	  0x06,0x01,0xd0,0x00,0x00, 0x06,0x01,0xe0,0x00,0x00, 0x06,0x01,0xff,0x00,0x00,	//;5,6	L13~L18
	  0x07,0x01,0xd8,0x00,0x00, 0x07,0x01,0xe8,0x00,0x00, 0x07,0x01,0xff,0x00,0x00,
	  0x08,0x01,0xd8,0x00,0x00, 0x08,0x01,0xe8,0x00,0x00, 0x08,0x01,0xff,0x00,0x00,	//;7,8	L19~L24
   	  0x09,0x01,0xff,0x00,0x00,
	  0x0a,0x01,0xff,0x00,0x00,

	  0x0b,0x01,0xff,0x00,0x00,

	  0x0c,0x01,0xff,0x00,0x00,						//;9~12	L25~L28
	  0x0d,0x01,0xff,0x00,0x00,

	  0x0e,0x01,0xff,0x00,0x00,
	  0x0f,0x01,0xff,0x00,0x00,
	  0x10,0x01,0xff,0x00,0x00,						//;13~16	L29~L32
   	  0x11,0x01,0xff,0x00,0x00,
	  0x12,0x01,0xff,0x00,0x00,


	  0x13,0x01,0xff,0x00,0x00,

	  0x14,0x01,0xff,0x00,0x00,						//;17~24	L33~L52

	  0x15,0x01,0xff,0x00,0x00,

	  0x16,0x01,0xff,0x00,0x00,
	  0x17,0x01,0xff,0x00,0x00,
	  0x18,0x01,0xff,0x00,0x00,


	  0x18,0x01,0x40,0x01,0x00,
	  0x18,0x01,0x80,0x01,0x00,
    	};

	unsigned char SONYCCD_Exposure50[416]={
	//

	  0x01,0x01,0xb0,0x00,0x00, 0x01,0x01,0xe0,0x00,0x00, 0x01,0x01,0xff,0x00,0x00,
	  0x02,0x01,0xb0,0x00,0x00, 0x02,0x01,0xe0,0x00,0x00, 0x02,0x01,0xff,0x00,0x00,	//;1~4	L1~L12

   	  0x03,0x01,0xb0,0x00,0x00, 0x03,0x01,0xe0,0x00,0x00, 0x03,0x01,0xff,0x00,0x00,
	  0x04,0x01,0xc0,0x00,0x00, 0x04,0x01,0xd0,0x00,0x00, 0x04,0x01,0xe0,0x00,0x00,
 	  0x05,0x01,0xb0,0x00,0x00, 0x05,0x01,0xc0,0x00,0x00, 0x05,0x01,0xd0,0x00,0x00, 0x05,0x01,0xe0,0x00,0x00,

	  0x06,0x01,0xd0,0x00,0x00, 0x06,0x01,0xe0,0x00,0x00,	//;5,6	L13~L18

	  0x07,0x01,0xd8,0x00,0x00, 0x07,0x01,0xe8,0x00,0x00, 0x07,0x01,0xff,0x00,0x00,
	  0x08,0x01,0xd8,0x00,0x00, 0x08,0x01,0xe8,0x00,0x00, 0x08,0x01,0xff,0x00,0x00,	//;7,8	L19~L24


	  0x09,0x01,0xff,0x00,0x00,
	  0x0a,0x01,0xff,0x00,0x00,

	  0x0b,0x01,0xff,0x00,0x00,
	  0x0c,0x01,0xff,0x00,0x00,						//;9~12	L25~L28

	  0x0d,0x01,0xff,0x00,0x00,

	  0x0e,0x01,0xff,0x00,0x00,

	  0x0f,0x01,0xff,0x00,0x00,
	  0x10,0x01,0xff,0x00,0x00,						//;13~16	L29~L32

	  0x11,0x01,0xff,0x00,0x00,
	  0x12,0x01,0xff,0x00,0x00,
	  0x13,0x01,0xff,0x00,0x00,
	  0x14,0x01,0xff,0x00,0x00,						//;17~24	L33~L52
	  0x14,0x01,0xff,0x00,0x00,
	  0x14,0x01,0xff,0x00,0x00,

	  0x14,0x01,0xff,0x00,0x00,
	  0x14,0x01,0xff,0x00,0x00,
	  0x14,0x01,0x40,0x01,0x00,
	  0x14,0x01,0x80,0x01,0x00,

	};

#ifdef ISSC
  if (Frequency == 50){
    if (FrameRate == 30)
      MaxExposure = 8;

    else if (FrameRate == 20)
      MaxExposure = 15;
    else
      MaxExposure = 17;
  }
  else if (Frequency == 60){
    if (FrameRate == 30)
      MaxExposure = 11;
    else if (FrameRate == 20)
      MaxExposure = 17;
    else
        MaxExposure = 23;
  }
  else
    MaxExposure = 24;
#else
  if (Frequency)
    MaxExposure = 41; 
  else
    MaxExposure = 24;
#endif
  
  if (Exposure < 0)

      Exposure = 0;

  if (Exposure > MaxExposure)
      Exposure = MaxExposure;
	unsigned char *ccdTemp;


	unsigned char RegData,RegData2;
  unsigned char CCDTemp1,CCDTemp2;
	long CCDExposure;

  



				if (Exposure < 1)
					Exposure = 1;

				if (Exposure > MaxExposure)
					Exposure = MaxExposure;
//			printk("CCD Exposure %d\n", Exposure);

#ifdef ISSC


        if (FrameRate == 30)
          temp3 = 0x02;
        else if (FrameRate == 20)
          temp3 = 0x05;
        else
          temp3 = 0x07;
#else
				if (Frequency == 50){
					if (Exposure <=8)        //30fps

						temp3 = 0x03;  //0x86

					else if (Exposure <=15)   //20fps
						temp3 = 0x05;

					else if (Exposure <=17)   //15fps



						temp3 = 0x07;
					else if (Exposure <=25)   //10 fps
						temp3 = 0x0b;

					else if (Exposure <=28)   //7.5 fps
						temp3 = 0x0f;
					else
						temp3 = 0x25;
				}
				else if (Frequency == 60){
					if (Exposure <=11)
						temp3 = 0x03;  //0x86
					else if (Exposure <=17)
						temp3 = 0x05;
					else if (Exposure <=23)

						temp3 = 0x07;

					else if (Exposure <=27)
						temp3 = 0x0b;
					else if (Exposure <=31)

						temp3 = 0x0f;
					else
						temp3 = 0x25;
				}
				else{
					temp3 = 0x07;
				}
#endif
        
        if(temp3 == 0x03)
					temp4 = 12;
				else if(temp3 == 0x05)
					temp4 = 8;
				else if(temp3 == 0x07)
					temp4 = 6;
				else if(temp3 == 0x0b)


					temp4 = 4;

				else if(temp3 == 0x0f)
					temp4 = 3;

				else if(temp3 == 0x25)
					temp4 = 2;




			if(Frequency == 50){
					CCDExposure = 0x20c - (SONYCCD_Exposure50[Exposure*5] * temp4 * 1000000 / (Frequency * 2) - 447) / 780;
			}
			else if(Frequency == 60){
					CCDExposure = 0x20c - (SONYCCD_Exposure60[Exposure*5] * temp4 * 1000000 / (Frequency * 2) - 447) / 780;

			}
      else
					CCDExposure = 0x20c - Exposure * 5;
                



			CCDTemp1 = (CCDExposure << 4) & 0xf0;
			CCDTemp2 = (CCDExposure & 0xffff) >> 4;
                
				RegData = topro_read_reg(udev, MCLK_SEL);


				if(RegData != temp3){
					topro_write_reg(udev, MCLK_SEL, temp3);
					topro_write_reg(udev, MCLK_CFG, 0x03);

          

					topro_write_i2c_ccd(udev, 0x15, 0x01, 0x00, 0x00);
					for(i=0;i<100;i++){  //Waiting for Sif_Control be reseted;
						RegData2 = topro_read_reg(udev, SIF_CONTROL);
						if(RegData2 == 0x00){


							break;

						}


					}
//					DBGOUT5(("Wait FRAME_RATE %d ms",i));
				}






				if (Frequency){

						if (Exposure >= 33){
							topro_write_i2c_ccd(udev, 0x36, 0x13, 0x00, 0x00);

						}
						else{


							topro_write_i2c_ccd(udev, 0x36, 0x21, 0x00, 0x00);
						}



						for(i=0;i<100;i++){  //Waiting for Sif_Control be reseted;
							RegData2 = topro_read_reg(udev, SIF_CONTROL);
							if(RegData2 == 0x00){
								break;
							}

					}
				}
#ifdef ISSC
#else       
//noise reduction control

#endif
//				DBGOUT5(("PreExposure Exposure %d %d",DC->PreExposure, DC->Exposure));


				PreExposure = Exposure;
				if(Frequency){
					if(Frequency == 50)
						ccdTemp = SONYCCD_Exposure50 + Exposure*5;
					else
						ccdTemp = SONYCCD_Exposure60 + Exposure*5;
          ExposureTime = ccdTemp;


          topro_write_i2c(udev, 0x63,	0x01, CCDTemp1, CCDTemp2);
					topro_write_i2c(udev, *(ccdTemp+1), *(ccdTemp+2), *(ccdTemp+3), *(ccdTemp+4));
//					DBGOUT5(("CCD Exposure %d Clk %x ", Exposure, temp3));

//					DBGOUT5(("WriteI2C %x %x %x %x ", *ccdTemp,		*(ccdTemp+1), *(ccdTemp+2), *(ccdTemp+3)));
//					DBGOUT5(("WriteI2C %x %x %x %x ",*(ccdTemp+4), *(ccdTemp+5), *(ccdTemp+6), *(ccdTemp+7)));
				}
        else{
          ExposureTime = 1;



          topro_write_i2c(udev, 0x63,	0x01, CCDTemp1, CCDTemp2);

					topro_write_i2c(udev, 0x01, 0xb0, 0x00, 0x00);

					topro_write_i2c(udev, 0x36, 0x21, 0x00, 0x00);
    			printk("63 01 %x %x\n", CCDTemp1, CCDTemp2);
        }


#endif

		//;		IrisPrevExposure= CurrentExposure; (;in topro_setiris())
}



unsigned char TICCD_Exposure60[135]={
	0x02,0x03,0xb0,0x00,0x00, 
	
	0x02,0x03,0xe0,0x00,0x00, 
	0x02,0x04,0xff,0x00,0x00,
	0x02,0x05,0xb0,0x00,0x00, 
	0x02,0x06,0xe0,0x00,0x00, 
	0x02,0x07,0xff,0x00,0x00,

	0x03,0x05,0xb0,0x00,0x00, 
	0x03,0x06,0xe0,0x00,0x00, 
	0x03,0x07,0xff,0x00,0x00,

	0x04,0x05,0xb0,0x00,0x00, 
	0x04,0x06,0xe0,0x00,0x00, 
	0x04,0x07,0xff,0x00,0x00,
	0x04,0x08,0xc0,0x00,0x00, 
	0x04,0x09,0xe0,0x00,0x00, 
	0x04,0x0a,0xff,0x00,0x00,
};

  unsigned char TICCD_Exposure50[135]={
    0x01,0x03,0xb0,0x00,0x00, 0x01,0x04,0xe0,0x00,0x00, 0x01,0x05,0xff,0x00,0x00,
	  0x02,0x06,0xb0,0x00,0x00, 0x02,0x07,0xe0,0x00,0x00, 0x02,0x08,0xff,0x00,0x00,
    0x03,0x06,0xb0,0x00,0x00, 0x03,0x07,0xe0,0x00,0x00, 0x03,0x08,0xff,0x00,0x00,
    };



//
void topro_autoexposure(struct pwc_device *pdev)	//950903
{
	int PrevExposure;
	long AccY;
	BYTE NextOff2GT,PrevOff2LT;

	static int FrameSkipCount=0;
	struct usb_device *udev= pdev->udev;		// 950903.951024 JJ-

	int RateAE;
	long calSmall, calLarge; 

	if (AutoExposure)
	;
	else
	{
		AccY = GetBulkInAvgY(udev);

		calSmall= min(AccYSmall, AccY);
		calLarge= max(AccYLarge, AccY);  //;	printk("TP6830:: Dis AE, Current Exposure= %d \n", CurrentExposure);

		if (calSmall!=AccYSmall || calLarge!=AccYLarge)
		{
			AccYSmall= calSmall;
			AccYLarge= calLarge;
			printk("DisAutoExposure (%d):: AccY, (-AccY+)= %d [%d %d] \n", CurrentExposure, AccY, AccYSmall, AccYLarge);
		}

		return;
	}

	FrameSkipCount++;

	AEThreshold = 120 + (pdev->exposurelevel - 10) * 10;
		//950903,[0,1,2,..10,.. 20] --> 
	//950903,[AEThreshold: 85,90,95,...135,... 185]
	//951114,[AEThreshold: 30,40,50,...130,... 230]


	if(CurrentExposure == 1)
		RateAE = 10;
	else if (CurrentExposure <= 62)				//17fps
		RateAE = 15;
	else
		RateAE = 4;

	if(Extra_RateAE > 0)
	{
		#ifdef _EXP_DBMSG_
		printk("extend RateAE(%d) -> %d \n", RateAE, RateAE + Extra_RateAE);
		#endif
		RateAE += Extra_RateAE;
	}

	if (FrameSkipCount % RateAE !=0)
		return;


	if(Extra_RateAE > 0)
	{
		Extra_RateAE = 0;
	}


	// Re-view point::
	AccY = GetBulkInAvgY(udev);

	if (AccY > 255 || AccY < 0){
		printk("AccY data Error\n");
		return;
	}

	#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 [(autoexposure)]
	#elif defined(TP6830_MT9V011)
	#elif defined(TP6830_SONYCCD)
	#else

	PrevExposure= CurrentExposure;

	NextOff2GT= Get_NextOfst(CurrentExposure);
	PrevOff2LT= Get_PrevOfst(CurrentExposure);
	//     [ExposureToDarker:0,    Normal: 10,  ExposureToLighter: 21]


	#ifdef _FLICKER_TOLERANCE_
	//if (AccY > AEThreshold + NextOff2GT + flicker_tolerance)
	if (AccY > AEThreshold + ((NextOff2GT + flicker_tolerance) * flicker_allow_tolerance) / 10)
	#else
	if (AccY > AEThreshold + NextOff2GT)
	#endif
	{
		// --> [AEThreshold: 85,90,95,...135,... 185]
		//   +10  --> [LargeThrshld: 95,100,105,...145,... 195]
		//   +10+9 --> [LargeThrshld: 104,109,114,...154,... 204]

		if (CurrentExposure==1)
		{
			CurrentIRISIndicator = 20;
		}
		
		if (CurrentExposure==1)
		{
		}
		else
		{
			#ifdef _FLICKER_TOLERANCE_
			flicker_current_direct = -1;
			#endif

			// I found a phenomenon that the CCD sensor will get much noise on the sudden exposure change.
			// Specially, from dark to light.
			#if 0
			if(CurrentExposure > 104 && AccY > 210)
			{
				CurrentExposure = 104;
				Extra_RateAE = 12;
			}
			else
			if(CurrentExposure > 95 && AccY > 210)
			{
				CurrentExposure = 95;
				Extra_RateAE = 12;
			}
			#else
			if(CurrentExposure > TI_13FPS_EXPO_NUM && AccY > 210)
			{
				CurrentExposure = TI_13FPS_EXPO_NUM;
				Extra_RateAE = 1;
			}
			else
			if(CurrentExposure > TI_15FPS_EXPO_NUM && AccY > 210)
			{
				CurrentExposure = TI_15FPS_EXPO_NUM;
				Extra_RateAE = 2;
			}
			else
			if(CurrentExposure > TI_20FPS_EXPO_NUM && AccY > 210)
			{
				CurrentExposure = TI_20FPS_EXPO_NUM;
				Extra_RateAE = 3;
			}
			else
			if(CurrentExposure > TI_25FPS_EXPO_NUM && AccY > 210)
			{
				CurrentExposure = TI_25FPS_EXPO_NUM;
				Extra_RateAE = 4;
			}
			#endif
			else
			if (AccY > AEThreshold + 50)
			{
				int diff = (AccY - AEThreshold)/8;
				CurrentExposure -= diff;
			}
			else
			if (AccY > AEThreshold + 40)
			{
				CurrentExposure -= 2;
			}
			else
			{
				CurrentExposure -= 1;
			}
		}
	}
	#ifdef _FLICKER_TOLERANCE_
	//else if ( AccY < AEThreshold - PrevOff2LT - flicker_tolerance)

	else if ( AccY < AEThreshold - ((PrevOff2LT - flicker_tolerance)*flicker_allow_tolerance)/10)
	
	#else
	else if ( AccY < AEThreshold - PrevOff2LT)
	#endif
	{
		// --> [AEThreshold:  85,90,95,...135,... 185]
		#ifdef _FLICKER_TOLERANCE_
		flicker_current_direct = 1;
		#endif

		if (CurrentExposure >= 68)
		{
			CurrentIRISIndicator = -9999;
		}
		else if (CurrentExposure < 68)
		{
			CurrentIRISIndicator = -20;
		}

		if (CurrentExposure == MaxExposure)
		{
		}
		else
		{
			if (AccY + 50 < AEThreshold)
			{
				int diff = (AccY - AEThreshold)/8;
				CurrentExposure -= diff;
			}
			else if (AccY < AEThreshold - 40)				// -10 --> [SmallThrshld: 75,80,85,...125,.. 175]
			{
				CurrentExposure += 3;
			}
			else if (AccY < AEThreshold - 30)				// -10 --> [SmallThrshld: 75,80,85,...125,.. 175]
			{
				CurrentExposure += 2;
			}
			else
			{
				CurrentExposure += 1; 
			}
		}
	}
	else
	{
		#ifdef _FLICKER_TOLERANCE_
		flicker_current_direct = 0;
		#endif
		CurrentIRISIndicator = 0;
	}


	//printk("flicker_tolerance: %d\n", flicker_tolerance);

	#ifdef _FLICKER_TOLERANCE_
	if(flicker_current_direct != 0 && flicker_current_direct != flicker_last_direct)
	{
		flicker_tolerance++;
	}
	else if(flicker_current_direct == 0 && flicker_current_direct != flicker_last_direct)
	{
		if(flicker_tolerance > 3)
		{
			flicker_tolerance-=3;
		}
		else
		{	
			flicker_tolerance=0;
		}
	}
	flicker_last_direct = flicker_current_direct;

	if(flicker_allow_tolerance == 10)
	{
		flicker_allow_tolerance = 18;
	}
	
	#endif	


	if (PrevExposure == CurrentExposure)    //; 20061018 JJ-
	{
		calSmall= min(AccYSmall, AccY);
		calLarge= max(AccYLarge, AccY);  

		if (calSmall!=AccYSmall || calLarge!=AccYLarge)
		{
			AccYSmall= calSmall;
			AccYLarge= calLarge;
			#ifdef _EXP_DBMSG_
			printk("viewExpo (%d):: AccY, (-AccY+)= %d [%d %d] \n", CurrentExposure, AccY, AccYSmall, AccYLarge);
			#endif
		}
		return;		
	}

	#ifdef _EXP_DBMSG_
	printk("\n");
	printk("Goal:: AccY, (AEC)= %d [[ %d %d ]], %s (%d ->%d) \n", 
		AccY, AEThreshold-PrevOff2LT, AEThreshold+NextOff2GT, 
		(AccY>(AEThreshold+NextOff2GT))?"DecExpos":
		(AccY<(AEThreshold-PrevOff2LT))?"IncExpos":"Keep", 
		PrevExposure, // "Keep" is no-way here
		CurrentExposure);
	#endif
#endif

//  if (CurrentExposure != PreExposure){
	topro_fps_control(udev, PrevExposure, CurrentExposure);
    	topro_setexposure(udev, CurrentExposure,pdev);
//  }
}

/////////////////////////////////////
//AutoWhiteBalance Function
/////////////////////////////////////

unsigned char AdjustGainData(unsigned char Dat)
{
	unsigned int WeiMulti;
	unsigned int SumDat= Dat;

	//;SumDat *= 0x20; // 0x30;
	SumDat *= 0x80;
	SumDat /= 0x40;

	//;20061114_JJ
	WeiMulti= (unsigned int) Get_AWBGainWei(CurrentExposure);
	SumDat *=  WeiMulti;
	SumDat /=  100;

	return (unsigned char) min(0xff,SumDat);
}

void topro_autowhitebalance(struct usb_device *udev)
{
  unsigned char DataR, DataB;
	unsigned char Data_G;
  long AccR=0,AccG=0,AccB=0;

  int RateCount = 10;
   
  if (!AutoWhiteBalance)
    return;

//  if(!Frequency)
//    return;   
 
 //unsigned char *pgain,*data1,*data2;
  FrameSkipCount++;

  if (FrameSkipCount % RateCount !=0)
    return;

//  topro_read_i2c(udev,0x5d,0x2c,&i2c1,&i2c2);
//  printk("i2c1 %x  i2c2 %x\n",i2c1,i2c2);

  GetBulkAvgRGB(udev, (UCHAR*)&AccB, (UCHAR*)&AccG, (UCHAR*)&AccR);

//#ifdef ISSC
  AccB -= 1;
//#endif  
	
#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 (Kill AWB)
  AccR= 187;  AccG= 187;  AccB= 187;
#endif

		if (abs(AccR - AccG) > 2){

			if (AccR > AccG){
				if (AccR > AccG + 20){
					AWB_FACTOR[0] -= min((int)AccR - (int)AccG, (int)4);
          if (AWB_FACTOR[0] < -63)
            AWB_FACTOR[0] = -63;
        }
				else
					AWB_FACTOR[0]--;
			}
			else{
				if (AccR < AccG - 20){
					AWB_FACTOR[0] += min((int)AccG - (int)AccR, 4);
          if (AWB_FACTOR[0] > 190)
            AWB_FACTOR[0] = 190;
        }
				else
					AWB_FACTOR[0]++;
			}
		}
		if (abs(AccB - AccG) > 2){
			if (AccB > AccG){
				if (AccB > AccG + 20){
					AWB_FACTOR[2] -= min((int)AccB - (int)AccG, (int)6);
          if (AWB_FACTOR[2] < -63)
            AWB_FACTOR[2] = -63;
        }
				else
					AWB_FACTOR[2]--;
			}
			else{

				if (AccB < AccG - 20){
					AWB_FACTOR[2] += min((int)AccG - (int)AccB, 6);
          if (AWB_FACTOR[2] > 190)
            AWB_FACTOR[2] = 190;
        }
				else
					AWB_FACTOR[2]++;
			}
		}
  
//adjust awb
  AWB_FACTOR[0] = max((int)-63,(int)AWB_FACTOR[0]);
  AWB_FACTOR[0] = min((int)190,(int)AWB_FACTOR[0]);
  AWB_FACTOR[2] = max((int)-63,(int)AWB_FACTOR[2]);
  AWB_FACTOR[2] = min((int)190,(int)AWB_FACTOR[2]);
//printk("R_FACTOR %d,B_FACTOR %d\n",AWB_FACTOR[0],AWB_FACTOR[2]);
       
  DataR = 0x40 + AWB_FACTOR[0];
  DataB = 0x40 + AWB_FACTOR[2];

	DataR= CheckGainData(DataR, AWB_GainR);
	DataB= CheckGainData(DataB, AWB_GainB);
	AWB_GainR= DataR;
	AWB_GainB= DataB;

#if 0
  topro_write_reg(udev, AWB_RGAIN, DataR);
  topro_write_reg(udev, AWB_BGAIN, DataB);  

	Data_G= topro_read_reg(udev, AWB_GGAIN);  // Equ to: PwrOn's Default value, AWB_GAIN_DEF(0x40)
#else

	Data_G= 0x40; //;Data_G= topro_read_reg(udev, AWB_GGAIN);

//; printk("TP6830:: AWB.GainRGB: %d>", DataR);
	DataR= AdjustGainData(DataR);
//; printk("%d", DataR);
//; printk(", %d>", Data_G);
	Data_G= AdjustGainData(Data_G);
//; printk("%d", Data_G);
//; printk(", %d>", DataB);
	DataB= AdjustGainData(DataB);
//; printk("%d", DataB);
//;	printk("\n");
  topro_write_reg(udev, AWB_RGAIN, DataR);
  topro_write_reg(udev, AWB_GGAIN, Data_G);
  topro_write_reg(udev, AWB_BGAIN, DataB);  
#endif


#if 1
 //;printk("TP6830:: AWB.AvgRGB:  %d, %d, %d\n", AccR,AccG,AccB);
 //;printk("TP6830:: AWB.GainRGB: %d, %d, %d\n", DataR,Data_G,DataB);
#endif

  return;
}

#define CHK_GAIN_WT		5
unsigned char CheckGainData(unsigned char Calc, unsigned char ModifyRecord)
{
	unsigned char DstData;
	int CheckData;

	if (Calc>ModifyRecord) //; 調大
		{
		CheckData= min(ModifyRecord+CHK_GAIN_WT, 255);
		DstData= min(Calc, CheckData);
		}
	else //; 調小
		{
		CheckData= ModifyRecord-CHK_GAIN_WT;
		CheckData= max(CheckData, 0); //; or 1
		DstData= max(Calc, CheckData);
		}
	return DstData;
}

int GetBulkInAvgY(struct usb_device *udev)
{

//	unsigned char pBuffer1[64],pBuffer2[64];
	unsigned char pBuffer1[64];
  UCHAR *pBuffer;

//	unsigned char QT_CFG;
	int Y_acc[4][4];
	int Factor[4][4]={
		{6,6,6,6},
		{6,6,6,6},
		{6,6,6,6},
		{6,6,6,6}
	};
	long SumY=0,SumFactor=0;
	int i,j,PixelNos;

//Y Accumulator
	topro_write_reg(udev, BULKIN_INI_ADR, 0xB0);  //bulk_in_addr
	topro_write_reg(udev, RD_ADR_START, 0x00);  //Y
	topro_bulk_in(udev, 0x00, pBuffer1,64);  //TP6830 bulkin 64 bytes data

  pBuffer = pBuffer1;
  //Gather every 8 points and one block is one forth of BitmapWidth
  PixelNos = (BitmapWidth / 4 / 8) * ( BitmapHeight / 4 / 8);
  
  for(i=0;i<4;i++){
    for(j=0;j<4;j++){
      Y_acc[i][j] = (*(pBuffer+1) << 8 | *pBuffer) * 2 / PixelNos; //Y_acc was divided by 2
      pBuffer += 2;
    }  
  }  
  
  for(i=0;i<4;i++){
    for(j=0;j<4;j++){

      if(Y_acc[i][j] < 80)  
		Factor[i][j] -= (125 - Y_acc[i][j]) / 10;

	  Factor[i][j] = max(1, Factor[i][j]);

	  SumFactor += Factor[i][j];
	  SumY += Y_acc[i][j] * Factor[i][j];      
    }
  }   

  SumY /= SumFactor;
  return SumY;
}

void GetBulkAvgRGB(struct usb_device *udev, unsigned char* AvgB, unsigned char* AvgG, unsigned char* AvgR)

{
//	 Cols
//	----------------- -------- -------- -----

//	|B00	|		|		|		|		|      (DL+DH), (DL+DH), (DL+DH), (DL+DH),
//	----------------- -------- -------- -----
//	|		|		|		|		|		|
//	|		|	B11	|		|		|		|      (DL+DH), (DL+DH), (DL+DH), (DL+DH),
//	----------------- -------- -------- -----
//	|		|		|		|		|		|

//	|		|		|	B23	|		|		|      (DL+DH), (DL+DH), (DL+DH), (DL+DH),
//	----------------- -------- -------- -----

//	|		|		|		|		|		|

//	|128x96	|		|		|	B33	|              (DL+DH), (DL+DH), (DL+DH), (DL+DH)=  共 32 bytes
//	----------------------------------------

	unsigned char Buffer1[64],Buffer2[64];
  UCHAR *pBufferY, *pBufferU, *pBufferV;
//	unsigned char QT_CFG;
	int Fact[4][4], SumFact=0;
	int Y_acc[4][4], U_acc[4][4], V_acc[4][4];
	long SumY=0, SumU=0, SumV=0;
	long Sum_R, Sum_G, Sum_B;

//	NTSTATUS ntStatus;
	int i,j,PixelNos;

//Y Accumulator

	topro_write_reg(udev, BULKIN_INI_ADR, 0xB0);  //bulk_in_addr

	topro_write_reg(udev, RD_ADR_START, 0x00);  //Y
	topro_bulk_in(udev, 0x00, Buffer1,64);  //TP6830 bulkin 64 bytes data (每次)
	topro_write_reg(udev, RD_ADR_START, 0x40);  //U  
	topro_bulk_in(udev, 0x00 , Buffer2, 64);


  
  PixelNos = (BitmapWidth / 4 / 8) * ( BitmapHeight / 4 / 8);
  pBufferY = Buffer1;
  pBufferU = Buffer1 + 32;
  pBufferV = Buffer2;
  
  for(i=0;i<4;i++){
    for(j=0;j<4;j++){
      Y_acc[i][j] = (*(pBufferY+1) << 8 | *pBufferY) * 2 / PixelNos; //Y_acc was divided by 2

      U_acc[i][j] = (*(pBufferU+1) << 8 | *pBufferU) * 2 / PixelNos - 128; //Y_acc was divided by 2
      V_acc[i][j] = (*(pBufferV+1) << 8 | *pBufferV) * 2 / PixelNos - 128; //Y_acc was divided by 2
      
      pBufferY += 2;
      pBufferU += 2;
      pBufferV += 2;
      
    }
  } 
  
	for(i=0 ; i<4 ;i++){
		for(j=0 ; j<4 ;j++){
		Fact[i][j] = 1;
  		SumFact += Fact[i][j];
		}
	}

	for(i=0 ; i<4 ;i++){
		for(j=0 ; j<4 ;j++){
			SumY += Y_acc[i][j] * Fact[i][j];
			SumU += U_acc[i][j] * Fact[i][j];
			SumV += V_acc[i][j] * Fact[i][j];
		}
	}

	SumY /= SumFact;
	SumU /= SumFact;
	SumV /= SumFact;

#ifdef TP6830_MT9M001  // (Rondall 20061011)
	//; *AvgR = SumY + SumV * 14022 / 10000;
	//; *AvgG = SumY - SumU * 3456 / 10000 - 7145 * SumV / 10000;
	//; *AvgB = SumY + 17710 * SumU / 10000;
	 *AvgR = SumY + (11398*SumV) / 10000;
	 *AvgG = SumY + (3947*SumU) / 10000 + (5806*SumV) / 10000;
	 *AvgB = SumY + (20321 * SumU) / 10000;
	*AvgR= max(0, min(255, *AvgR));
	*AvgG= max(0, min(255, *AvgG));
	*AvgB= max(0, min(255, *AvgB));
#elif defined(TP6830_TICCD)  // (Rondall 20061011)

	 Sum_R = SumY + (11398*SumV) / 10000;
	 Sum_G = SumY + (3947*SumU) / 10000 + (5806*SumV) / 10000;
	 Sum_B = SumY + (20321 * SumU) / 10000;
	// (Rondall, 20061016)
	Sum_R= max(0, min(255, Sum_R));
	Sum_G= max(0, min(255, Sum_G)); 
	Sum_B= max(0, min(255, Sum_B)); 
	*AvgR= Sum_R;
	*AvgG= Sum_G;
	*AvgB= Sum_B;

#else
	 *AvgR = SumY + SumV * 14022 / 10000;
	 *AvgG = SumY - SumU * 3456 / 10000 - 7145 * SumV / 10000;
	 *AvgB = SumY + 17710 * SumU / 10000;
	*AvgR= max(0, min(255, *AvgR));
	*AvgG= max(0, min(255, *AvgG));
	*AvgB= max(0, min(255, *AvgB));
#endif
	
//	printk("Avg RGB %d %d %d\n", *AvgR, *AvgG, *AvgB);
	return;
}



void topro_SetAutoQuality(struct usb_device *udev){  //950209


  topro_write_reg(udev, B_FULL1_TH, 0x08);
  topro_write_reg(udev, B_FULL2_TH, 0x80);
  topro_write_reg(udev, AUTOQ_FUNC, 0x49);

  udelay(1);
  
  topro_write_reg(udev, B_FULL2_TH, 0x80);
  topro_write_reg(udev, B_FULL1_TH, 0x08);

  printk("SetAutoQuality\n");
}


//////////////////////////////////////////////////////////////
//topro_setquality use this fn to set Jpeg compression rate
//Higher QUALITY value cause Higher compression rate(lower image quality)
//////////////////////////////////////////////////////////////
void topro_setquality(struct usb_device *udev, int quality){
  unsigned char index;
  
  if (quality > 15)
    index = 15;
  else if (quality < 0)
    index = 0;
  else
    index = quality;
  
  topro_write_reg(udev, AUTOQ_FUNC, 0x00);  
  topro_write_reg(udev, JPEG_FUNC, index);



}


unsigned long SwDetectMotion(struct pwc_device *pdev){
//	 Cols
//	----------------- -------- -------- -----
//	|128X128|		|		|		|		|
//	|B00	|		|		|		|		|
//	----------------- -------- -------- -----
//	|		|		|		|		|		|
//	|		|	B11	|		|		|		|
//	----------------- -------- -------- -----
//	|		|		|		|		|		|
//	|		|		|	B23	|		|		|
//	----------------- -------- -------- -----

//	|		|		|		|		|		|
//	|128x96	|		|		|	B33	|
//	----------------------------------------
	UCHAR pBuffer1[64];
  UCHAR *pBuffer;
	unsigned long MotionPos;

	int RateMotion;

	int Y_acc[16];



	int i,PixelNos;
  struct usb_device *udev;

  udev = pdev->udev;

//	if(!DC->SwMotion)
//		return;

	if(ExposureTime <= 0x05){
		RateMotion = 15 * FrameRate / 30;
	}
	else if(ExposureTime <= 0x0a){
		RateMotion = 8 * FrameRate / 30;
	}
	else
		RateMotion = 5 * FrameRate / 30;

  if (!RateMotion)
    RateMotion = 1;


	if(pdev->vframe_count % RateMotion != 0)
		return 0;

	topro_write_reg(udev, BULKIN_INI_ADR, 0xB0);  //bulk_in_addr

	topro_write_reg(udev, RD_ADR_START, 0x00);  //Y
	topro_bulk_in(udev, 0x00, pBuffer1,64);  //TP6830 bulkin 64 bytes data
  pBuffer = pBuffer1;
  //Gather every 8 points and one block is one forth of BitmapWidth
  PixelNos = (BitmapWidth / 4 / 8) * ( BitmapHeight / 4 / 8);

  for(i=0;i<16;i++){
      Y_acc[i] = (*(pBuffer+1) << 8 | *pBuffer) * 2 / PixelNos; //Y_acc was divided by 2
      pBuffer += 2;
  }


	MotionPos = 0;
	for(i=0 ; i<16 ;i++){
		if(Y_acc[i] > 30){


			if(abs(BlockAvgY[i] - Y_acc[i]) > Motion_Ythrld)

				MotionPos |= (0x1 << i)	;
			BlockAvgY[i] = Y_acc[i];

		}
  }


  if (MotionPos){
  	for(i=0 ; i<16 ;i++){
      if (i % 4 == 0)
        printk("\n");
      if ((MotionPos >> i) & 0x01)
        printk("1 ");
      else
        printk("0 ");
   	}
  }


//	WriteStatusToRegistry(L"MotionPos", MotionPos);

	return MotionPos;
}




void topro_set_parameter(struct usb_device *udev)
{

  if (BitmapWidth == 640){
    jpg_hd[509] = 0x01;
    jpg_hd[510] = 0xe0;

    jpg_hd[511] = 0x02;
    jpg_hd[512] = 0x80;
  }
  else if (BitmapWidth == 320){
    jpg_hd[509] = 0x00;
    jpg_hd[510] = 0xf0;
    jpg_hd[511] = 0x01;
    jpg_hd[512] = 0x40;
  }
  else{        //176
    jpg_hd[509] = 0x00;
    jpg_hd[510] = 0x90;
    jpg_hd[511] = 0x00;
    jpg_hd[512] = 0xb0;

  }

#ifdef TP6830_MT9M001 	// micron TP6830_MT9M001 (_set_parameter)

  if(BitmapWidth == 640){   // VGA parameter

	  if (topro_prog_regs(udev, vga_micron_MT9M001_vga))  //930406
		  printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);        

  }
  //; else if(BitmapWidth == 176){  // TP6830_QCIF parameter
	//;   if (topro_prog_regs(udev, qcif_micron_MT9M001_qcif))  //951011 JJ--
	//; 	  printk("program qcif data error.\n");								//
  //;   topro_write_reg(udev, SCALE_CFG, 0x11);								//scaling down horizontal&vertical
  //; }
  else if(BitmapWidth == 176){																//TP6830_QCIF parameter
		printk("1.set_parameter, 176/qcif registers\n");
	  if (topro_prog_regs(udev, qcif_micron_MT9M001_qcif))			//951011 JJ--
	 	 printk("program qcif data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);										//scaling down horizontal&vertical
		printk("2.scaling down horizontal&vertical \n");
		printk("2.set_parameter, 176/qcif registers\n");
  }
  else{  // QVGA parameter
	  if (topro_prog_regs(udev, qvga_micron_MT9M001_qvga))  //930406
		  printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);  //scaling down horizontal&vertical
  }

#elif defined TP6830_MT9V011


  if(BitmapWidth == 640){

  // VGA parameter
	  if (topro_prog_regs(udev, topro_vga_micron_MT9V011))  //930406
		  printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);
  }

  else if(BitmapWidth == 176){ //TP6830_QCIF
	  if (topro_prog_regs(udev, topro_qcif_micron_MT9V011))  //930406
		  printk("program qcif data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x11);
  }
  else{
	  if (topro_prog_regs(udev, topro_qvga_micron_MT9V011))  //930406
		  printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);

  }

#elif defined TP6830_SHARPCCD
  // tp68xx initialize data
	if (topro_prog_regs(udev, Sharp_CCD))
		printk("program initial data error.\n");





  if(BitmapWidth == 640){
  // VGA parameter
  	if (topro_prog_regs(udev, Sharp_CCD_VGA))  //930406
	  	printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x00);

  }
  else if(BitmapWidth == 176){    //TP6830_QCIF
  // VGA parameter
  	if (topro_prog_regs(udev, Sharp_CCD_QCIF))  //930406
	  	printk("program vga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);

  }

  else{

  // QVGA parameter

  	if (topro_prog_regs(udev, Sharp_CCD_QVGA))  //930406
	  	printk("program qvga data error.\n");
    topro_write_reg(udev, SCALE_CFG, 0x11);


  }


#elif defined TP6830_SONYCCD
  // tp68xx initialize data
	if (topro_prog_regs(udev, Sony_CCD))
		printk("program initial data error.\n");







  if(BitmapWidth == 640){
  // VGA parameter

  	if (topro_prog_regs(udev, Sony_CCD_VGA))  //930406
	  	printk("program vga data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x00);

  }
  else if(BitmapWidth == 176){
  // QVGA parameter
  	if (topro_prog_regs(udev, Sony_CCD_QCIF))  //930406
	  	printk("program qvga data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x11);


  }  
  else{
  // QVGA parameter
  	if (topro_prog_regs(udev, Sony_CCD_QVGA))  //930406
	  	printk("program qvga data error.\n");

    topro_write_reg(udev, SCALE_CFG, 0x11);

  }

#endif

}

void topro_set_edge_enhance(struct usb_device *udev)
{

	UCHAR Data;
	UCHAR En[5] = {1,1,1,1,1};
	UCHAR Edge[8];


	Edge[0] = 0x07;  //SHARP_STRENGTH

	Edge[1] = 0x01;  //ESHARP_HIGH_SCAL
	Edge[2] = 0x01;  //SHARP_LOW_SCALE
	Edge[3] = 0x03;	 //SHARP_CORING_TH
	Edge[4] = 0x02;  //BLUR_Y_FUNC
	Edge[5] = 0x07;  //BLUR_UV_FUNC
	Edge[6] = 0x03;  //DENOISE_Y_FUNC
	Edge[7] = 0x03;  //DENOISE_YV_FUNC

	Data = En[0];
	topro_write_reg(udev, SHARP_EN, (UCHAR)Data);

	Data = (Edge[2] << 6)  | (Edge[1] << 4) | Edge[0];
	topro_write_reg(udev, SHARP_CFG, Data);


	Data = Edge[3];
	topro_write_reg(udev, SHARP_CORING_TH, Data);

	Data = Edge[5] << 5 | En[2] << 4 | Edge[4] << 1 | En[1];
	topro_write_reg(udev, BLUR_CFG, Data);

	Data = Edge[7] << 5 | En[4] << 4 | Edge[6] << 1 | En[3];
	topro_write_reg(udev, DENOISE_CFG, Data);

	return;

}


int topro_set_osd(struct usb_device *udev, int index)
{
  int i;
  int byteno;
  unsigned char grp1_x = 0;  //4;  
  unsigned char grp2_x = 11; //16; 
  unsigned char grp3_x = 20; //28; 
  unsigned char grp4_x = 28; //40; 
	unsigned char 	grp1_y = 15; //15
	unsigned char 	grp2_y = 10; //15
	unsigned char 	grp3_y =  5; //15
	unsigned char 	grp4_y =  0; //15

  unsigned char font_color_y = 0xff,
								font_color_u = 0x80, //0xe0, //0x80,
								font_color_v = 0xe0; //0x80; 

  if(index == 0){
	  topro_write_reg(udev, 0xD0, 0x00);
    return 0;
  }

  switch(index){
  case 1:
    byteno = sizeof(font1);
    topro_write_reg(udev, 0xD0, 0x02);   
    for(i=0;i<(byteno-1);i++){
	    topro_write_reg(udev, 0xDC, font1[i]);
    }
    break;
  case 2:
    byteno = sizeof(font2);
    topro_write_reg(udev, 0xD0, 0x02);
    for(i=0;i<(byteno-1);i++){
	    topro_write_reg(udev, 0xDC, font2[i]);
    }
    break;
  case 3:
    byteno = sizeof(font3);
    topro_write_reg(udev, 0xD0, 0x02);
    for(i=0;i<(byteno-1);i++){
	    topro_write_reg(udev, 0xDC, font3[i]);
    }
    break;
  case 4:
    byteno = sizeof(font4);
    topro_write_reg(udev, 0xD0, 0x02);
    for(i=0;i<(byteno-1);i++){
	    topro_write_reg(udev, 0xDC, font4[i]);
    }
    break;  
  }

	topro_write_reg(udev, 0xD1, grp1_x);
	topro_write_reg(udev, 0xD5, grp1_y);

	topro_write_reg(udev, 0xD2, grp2_x);
	topro_write_reg(udev, 0xD6, grp2_y);

	topro_write_reg(udev, 0xD3, grp3_x);
	topro_write_reg(udev, 0xD7, grp3_y);

	topro_write_reg(udev, 0xD4, grp4_x);
	topro_write_reg(udev, 0xD8, grp4_y);

	topro_write_reg(udev, 0xD9, font_color_y);
	topro_write_reg(udev, 0xDA, font_color_u);
	topro_write_reg(udev, 0xDB, font_color_v);  

  topro_write_reg(udev, 0xD0, 0x01);
  return 1;
}

void update_rtc_time(struct usb_device *udev, Tp_Rtc_Data *pRtc_Data)
{
  int year= pRtc_Data->year;
  unsigned char temp, 
		month=pRtc_Data->month,
		day=pRtc_Data->day,
		hour=pRtc_Data->hour,
		min=pRtc_Data->minute,
		sec=pRtc_Data->second;
  unsigned char BCD_Time[7];

	temp = (unsigned char)(year/100);
	BCD_Time[0] = (temp/10)*16 + (temp%10);

	temp = (unsigned char)(year%100);
	BCD_Time[1] = (temp/10)*16 + (temp%10);

	temp = month;
	BCD_Time[2] = (temp/10)*16 + (temp%10);

	temp = day;
	BCD_Time[3] = (temp/10)*16 + (temp%10);

	temp = hour;
	BCD_Time[4] = (temp/10)*16 + (temp%10);

	temp = min;
	BCD_Time[5] = (temp/10)*16 + (temp%10);

	temp = sec;
	BCD_Time[6] = (temp/10)*16 + (temp%10);

  topro_write_reg(udev, YEAR_H, BCD_Time[0]);
	topro_write_reg(udev, YEAR_L, BCD_Time[1]);

	topro_write_reg(udev, MONTH, BCD_Time[2]);

	topro_write_reg(udev, DAY, BCD_Time[3]);

	topro_write_reg(udev, HOUR, BCD_Time[4]);

	topro_write_reg(udev, MIN, BCD_Time[5]);

	topro_write_reg(udev, SEC, BCD_Time[6]);
}

  
void topro_config_rtc(struct usb_device *udev)
{
	unsigned char RegData;
	unsigned char 
						rtc_en = ShowRTC,
          	year_order = 0,
          	clock_pmam_order = 0,
          	clock_pmam_year_order = 0,
          	hour_format = 0,
          	char_space = 0;
	unsigned char 
						font_size_h = 1,
	          font_size_v = 2,
            locate_select,
            show_pmam = 1;
	unsigned char 
						time_locx = 0,
          	time_locy = 51, //5,
            date_locx = 0,  //0x18,
            date_locy = 55; //5;
	unsigned char 
          	rtd_y = 0xd8, //0xff,
          	rtd_u = 0x80, //0x80,
          	rtd_v = 0x30; //0x80;

	RegData = char_space << 5 | hour_format << 4 | clock_pmam_year_order << 3 | clock_pmam_order << 2 | year_order<< 1 | rtc_en;
	topro_write_reg(udev, TIME_CTL, RegData );
 
  locate_select = 0;
  RegData = locate_select << 5 | show_pmam << 4 | (2 - font_size_v) << 2 | (2 - font_size_h);
	topro_write_reg(udev, TIME_FORMAT, RegData);

	RegData = time_locx;
	topro_write_reg(udev, TIME_LOCX, RegData);

	RegData = time_locy;
	topro_write_reg(udev, TIME_LOCY, RegData);
  

  locate_select = 1;
  RegData = locate_select << 5 | show_pmam << 4 | (2 - font_size_v) << 2 | (2 - font_size_h);
	topro_write_reg(udev, TIME_FORMAT, RegData);

	RegData = date_locx;
	topro_write_reg(udev, TIME_LOCX, RegData);
	RegData = date_locy;
	topro_write_reg(udev, TIME_LOCY, RegData);

	RegData = rtd_y;
	topro_write_reg(udev, RTD_Y, RegData);
	RegData = rtd_u;
	topro_write_reg(udev, RTD_U, RegData);
	RegData = rtd_v;
	topro_write_reg(udev, RTD_V, RegData);   
	
#if 1
	topro_write_reg(udev, SET_YUV, 0x6a | 0x01);  // JJ-
#endif
}


