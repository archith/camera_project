/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _IMAGE_BILK_OPS_H_
#define  _IMAGE_BILK_OPS_H_

#define IMAGE_BULK_NAME	"IMAGE"

#ifndef PACKED
#define PACKED          __attribute__((packed))
#endif /* PACKED */


#define MOBILE_ENABLE	1

#define  IMG_OK      0
#define  IMG_ERROR  -1

/* resolution */
#define  IMG_RES_120  1		/* 120 x 160 */
#define  IMG_RES_240  2		/* 240 x 320, default */
#define  IMG_RES_480  3		/* 480 x 640 */
#define  IMG_RES_D1   4		/* 480 x 704 */
#define  IMG_RES_48   7		/*  48 x  64  */
#define  IMG_RES_96   8         /*  96 x 128  */

/* quality level  */
#define  IMG_QUAL_VHI   1	/* very high */
#define  IMG_QUAL_HI    2	/* high */
#define  IMG_QUAL_MID   3	/* middle, default */
#define  IMG_QUAL_LOW   4	/* low */
#define  IMG_QUAL_VLOW  5	/* very low */

#define	 IMG_QUAL_MIN	IMG_QUAL_VHI	//very low
#define	 IMG_QUAL_MAX	IMG_QUAL_VLOW
#define	 IMG_QUAL_DFT	IMG_QUAL_MID

/* quality type */
#define  IMG_QUAL_BITRATE   0	// Fixed the bit rate
#define  IMG_QUAL_QUALITY   1	// Fixed the quality

#define  IMG_QUAL_TYPE_MIN	IMG_QUAL_BITRATE
#define  IMG_QUAL_TYPE_MAX	IMG_QUAL_QUALITY
#define  IMG_QUAL_DEFAULT   IMG_QUAL_QUALITY

/* bit rate */
#define  IMG_BITRATE_32K        0
#define  IMG_BITRATE_64K        1
#define  IMG_BITRATE_96K        2
#define  IMG_BITRATE_128K       3
#define  IMG_BITRATE_256K       4
#define  IMG_BITRATE_384K       5
#define  IMG_BITRATE_512K       6
#define  IMG_BITRATE_768K       7
#define  IMG_BITRATE_1024K      8
#define  IMG_BITRATE_1280K      9

#define IMG_BITRATE_MIN		IMG_BITRATE_64K
#define IMG_BITRATE_MAX		IMG_BITRATE_1280K

/* timestamp */
#define  IMG_TIMESTAMP_ENABLE		1
#define  IMG_TIMESTAMP_DISABLE		0
#define  IMG_TIMESTAMP_DEFAULT		IMG_TIMESTAMP_DISABLE

/* text overlay */
#define  IMG_TEXTOVERLAY_ENABLE		1
#define  IMG_TEXTOVERLAY_DISABLE	0
#define  IMG_TEXTOVERLAY_DEFAULT	IMG_TEXTOVERLAY_DISABLE

#define  IMG_TEXTOVERLAY_VALUE		""

#define  IMG_MSG_LEN  20

/* image overlay */
#define  IMG_LOGO_ENABLE	1
#define  IMG_LOGO_DISABLE	0
#define  IMG_LOGO_DEFAULT	IMG_LOGO_DISABLE

#define	 IMG_LOGO_MANUFACTURER	0
#define  IMG_LOGO_USER		1

#define  IMG_LOGO_OPTION_MIN	IMG_LOGO_MANUFACTURER
#define  IMG_LOGO_OPTION_MAX	IMG_LOGO_USER


#define  IMG_LOGO_TRANSPARENT_DISABLE	0
#define  IMG_LOGO_TRANSPARENT_ENABLE	1
#define  IMG_LOGO_TRANSPARENT_DEFAULT	IMG_LOGO_TRANSPARENT_DISABLE

/* time stamp and msg location */
#define  IMG_LOC_UL    1	/* upper left, default */
#define  IMG_LOC_UR    2	/* upper right */
#define  IMG_LOC_LL    3	/* lower left */
#define  IMG_LOC_LR    4	/* lower right */

/* privacy mask color */
#define COLOR_MIN_NUM	COLOR_NONE
#define COLOR_NONE	0
#define COLOR_WHITE	1
#define COLOR_BLACK	2
#define COLOR_GRAY	3
#define COLOR_BLUE	4
#define COLOR_CYAN	5
#define COLOR_GREEN	6
#define COLOR_YELLOW	7
#define COLOR_BROWN	8
#define COLOR_ORANGE	9
#define COLOR_RED	10
#define COLOR_VIOLET	11
#define COLOR_PURPLE	12
#define COLOR_MAX_NUM	COLOR_PURPLE

/* image adjustment for hue, brightness, contrast */
#define  IMG_ADJ_MAX  64
#define  IMG_ADJ_DFT  32
#define  IMG_ADJ_MIN   0

#define  IMG_BRIGHTNESS_DFT  4

// sharpness, hue, saturation, contrast adjust range
// range 0 to 6, map to web ui -3 to 3
#define  IMG_ADJ2_MAX    7
#define  IMG_ADJ2_DFT    4
#define  IMG_ADJ2_MIN    1

/* flip */
#define  IMG_FLIP_ON	1
#define  IMG_FLIP_OFF	0
#define  IMG_FLIP_DEFAULT	IMG_FLIP_OFF

/* mirror */
#define  IMG_MIRROR_DEFAULT	IMG_MIRROR_OFF
#define  IMG_MIRROR_ON		1
#define  IMG_MIRROR_OFF		0

/* frame rate */
#define  IMG_FRATE_MIN  0
#define  IMG_FRATE_MAX  30
#define  IMG_FRATE_DFT  IMG_FRATE_MAX


/* powerline frequency */
#define  IMG_PW_FREQUENCY_50HZ		50
#define  IMG_PW_FREQUENCY_60HZ		60
#define  IMG_PW_FREQUENCY_DEFAULT	IMG_PW_FREQUENCY_60HZ

#define  IMG_PW_FREQUENCY_MIN		IMG_PW_FREQUENCY_50HZ
#define  IMG_PW_FREQUENCY_MAX		IMG_PW_FREQUENCY_60HZ


/* bandwidth control */
#define  IMG_BANDWIDTH_64K		1
#define  IMG_BANDWIDTH_128K		2
#define  IMG_BANDWIDTH_256K		3
#define  IMG_BANDWIDTH_512K		4
#define  IMG_BANDWIDTH_768K		5
#define  IMG_BANDWIDTH_1M		6
#define  IMG_BANDWIDTH_1536K		7
#define  IMG_BANDWIDTH_2M		8
#define  IMG_BANDWIDTH_DEFAULT		0

enum {
  VDO_AWB_AUTO=0,
  VDO_AWB_INDOOR,
  VDO_AWB_FL_WHITE,
  VDO_AWB_FL_YELLOW,
//  VDO_AWB_OUTDOOR,
  VDO_AWB_BLACKWHITE,
}; /* auto white-balance table */

#define VDO_AWB_MIN_NUM	VDO_AWB_AUTO
#define VDO_AWB_MAX_NUM	VDO_AWB_BLACKWHITE

typedef struct BS_Conf
{
  int enable;			// 0: disable 1: enable
  int resolution;		// IMG_RES_xxx, default IMG_RES_240
  int quality_type;		// 0: fix bit rate 1: fix quality
  int quality_level;		// 1~5, default 3: Normal
  int frame_rate;		// frame rate, depends on resolution
  int bit_rate;			// bit rate
  int bandwidth;		// bandwith control
} BS_Conf;

#define CCD_SHMKEY          ((key_t) 7890)
#define CCD_PERMS           0666
#define CCD_PAGE_NUM        5

typedef struct
{
  int fps;
  int quality;
  int brightness;
  int contrast;
  int whiteness;
  int color;
  int hue;
  int saturation;
  int frequency;
  int auto_exposure;
  int white_balance;
  int sharpness;
} CCD_CONFIGURATION;

#define MP4_STREAM	0
#define JPG_STREAM	1
#define MOBILE_STREAM	2


struct IMAGE_DS
{
	/* encoders settings */
	BS_Conf bitstream[3];

	/* on screen display settings */
	int stmp;			// time stamp, 0-disable(default), 1-enable
	int text;			// msg display, 0-disable(default), 1-enable
	char msg[IMG_MSG_LEN + 1];	//  IMG_MSG_LEN characters
	char loc;			// time stamp and msg location, IMG_LOC_XX, default IMG_LOC_UL

	int image_overlay;		// 0: disable   1: enable
	int image_option;		// 0: default logo, 1: user define logo
	int image_position[2];
	int image_transparent;	//
	char image_trans_color[IMG_MSG_LEN + 1];


	int mask_window1;
	int mask_position1[2];
	int mask_dimension1[2];
	int mask_color1;
	int mask_window2;
	int mask_position2[2];
	int mask_dimension2[2];
	int mask_color2;
	int mask_window3;
	int mask_position3[2];
	int mask_dimension3[2];
	int mask_color3;
	int mask_window4;
	int mask_position4[2];
	int mask_dimension4[2];
	int mask_color4;

	/* sensor settings */
	int freq;			// power line frequency, default 60Hz
	int brit;			// Brightness, VDO_BRIGHTNESS_MIN~VDO_BRIGHTNESS_MAX, default VDO_DEF_BRIGHTNESS
	int shrp;			//sharpness, IMG_ADJ_MIN~IMG_ADJ_MAX, default IMG_ADJ_DFT
	int mirr;			// mirror
	int flip;			// rotation, IMG_ROT_0, IMG_ROT_90,...
	int ctype;			// Manual,Indoor,Fluorescent(white),Fluorescent(yellow),Outdoor

	// updated to support hue, saturation, contrast (951025)
	int hue;			// hue
	int saturation;		// saturation
	int contrast;			// Hue

	// update to support mobile access code
	char access_code[17];
};

int IMAGE_BULK_ReadDS(void* ds);
int IMAGE_BULK_CheckDS(void* ds, void* ds_org);
int IMAGE_BULK_WriteDS(void* ds, void* ds_org);
int IMAGE_BULK_RunDS(void* ds, void* ds_org);


#endif
