#ifndef PWC_IOCTL_H
#define PWC_IOCTL_H

/* (C) 2001 Nemosoft Unv.    webcam@smcc.demon.nl
   
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
   2001/08/03  Alvarado   Added ioctl constants to access methods for 
                          changing white balance and red/blue gains
 */

/* These are private ioctl() commands, specific for the Philips webcams.
   They contain functions not found in other webcams, and settings not
   specified in the Video4Linux API. 
   
   The #define names are built up like follows:
   VIDIOC		VIDeo IOCtl prefix
         PWC		Philps WebCam
            G           optional: Get
            S           optional: Set
             ... 	the function
 */




/* The frame rate is encoded in the video_window.flags parameter using
   the upper 16 bits, since some flags are defined nowadays. The following
   defines provide a mask and shift to filter out this value.
   
   In 'Snapshot' mode the camera freezes its automatic exposure and colour 
   balance controls.
 */
#define PWC_FPS_SHIFT		16
#define PWC_FPS_MASK		0x00FF0000
#define PWC_FPS_FRMASK		0x003F0000
#define PWC_FPS_SNAPSHOT	0x00400000

//950903
/* pwc_whitebalance.mode values */
#define PWC_WB_INDOOR		0
#define PWC_WB_OUTDOOR	1
#define PWC_WB_FL		    2
#define PWC_WB_FL_Y		  3
//#define PWC_WB_MANUAL		3
#define PWC_WB_AUTO		  4
#define PWC_WB_BW   	  5

/* Used with VIDIOCPWC[SG]AWB (Auto White Balance). 
   Set mode to one of the PWC_WB_* values above.
   *red and *blue are the respective gains of these colour components inside 
   the camera; range 0..65535
   When mode == PWC_WB_MANUAL, manual_red and manual_blue are set or read; 
   otherwise undefined.
   read_red and read_blue are read-only.
*/   
   
struct pwc_whitebalance
{
	int mode;
	int manual_red, manual_blue;	/* R/W */
	int read_red, read_blue;	/* R/O */
};


/* Used with VIDIOCPWC[SG]LED */
struct pwc_leds
{
	int led_on;			/* Led on-time; range = 0..25000 */
	int led_off;			/* Led off-time; range = 0..25000  */
};


#define GPIO_STATUS_LOW		0
#define GPIO_STATUS_HIGH	1
#define GPIO_XXX_NUM				7
#define GPIO_IR_FILTER_NUM	6

struct gpio_status {
	int gpio_number;
	int qpio_status;
};


 /* Restore user settings */
#define VIDIOCPWCRUSER		_IO('v', 192)
 /* Save user settings */
#define VIDIOCPWCSUSER		_IO('v', 193)
 /* Restore factory settings */
#define VIDIOCPWCFACTORY	_IO('v', 194)

 /* You can manipulate the compression factor. A compression preference of 0
    means use uncompressed modes when available; 1 is low compression, 2 is
    medium and 3 is high compression preferred. Of course, the higher the
    compression, the lower the bandwidth used but more chance of artefacts
    in the image. The driver automatically chooses a higher compression when
    the preferred mode is not available.
  */

 /* [Sercomm] */
#define VIDIOCPWCSCQUAL		_IOW('v', 195, int)   /* Set preferred compression quality (0 = uncompressed, 3 = highest compression) */
#define VIDIOCPWCGCQUAL		_IOR('v', 195, int)   /* Get preferred compression quality */

#define VIDIOCPWCSAGC		_IOW('v', 200, int)		/* Set AGC (Automatic Gain Control); int < 0 = auto, 0..65535 = fixed */
#define VIDIOCPWCGAGC		_IOR('v', 200, int)     /* Get AGC; int < 0 = auto; >= 0 = fixed, range 0..65535 */
#define VIDIOCPWCSSHUTTER	_IOW('v', 201, int)     /* Set shutter speed; int < 0 = auto; >= 0 = fixed, range 0..65535 */

 /* [Sercomm] */
#define VIDIOCPWCSAWB           _IOW('v', 202, struct pwc_whitebalance)   /* Color compensation (Auto White Balance) */
#define VIDIOCPWCGAWB           _IOR('v', 202, struct pwc_whitebalance)

#define VIDIOCPWCSLED           _IOW('v', 205, struct pwc_leds)   /* Turn LED on/off ; int range 0..65535 */
#define VIDIOCPWCGLED           _IOR('v', 205, struct pwc_leds)   /* Get state of LED; int range 0..65535 */

#define VIDIOCSETGAMMAR         _IOW('v', 210, unsigned char)
#define VIDIOCSETGAMMAG         _IOW('v', 211, unsigned char)
#define VIDIOCSETGAMMAB         _IOW('v', 212, unsigned char)
#define VIDIOCSETGAIN           _IOW('v', 213, unsigned char)

#define VIDIOCWRITEREG          _IOW('v', 214, unsigned char)
#define VIDIOCREADREG           _IOR('v', 215, unsigned char)

#define VIDIOCSETEXPOSURE       _IOW('v', 216, unsigned char)

 /* [Sercomm] */
#define VIDIOCGETMOTION         _IOW('v', 217, unsigned char)
#define VIDIOCSETMOTION         _IOW('v', 218, unsigned char)
#define VIDIOCGETFREQUENCY      _IOW('v', 219, unsigned char)
#define VIDIOCSETFREQUENCY      _IOW('v', 220, unsigned char)

 /* [Sercomm] */
//;copy_from_6810_lnx_driver
#define VIDIOCAUTOEXPOSURE      _IOW('v', 221, unsigned char)  // copy_from_6810_lnx_driver
#define VIDIOCAUTOQUALITY       _IOW('v', 222, unsigned char)  // copy_from_6810_lnx_driver JJ-

//;(reserved)
#define VIDIOCRESETCAMERA       _IOW('v', 223, unsigned char)  // copy_from_6810_lnx_driver JJ-

 /* [Sercomm] */
#define VIDIOCFRAMECOUNT        _IOW('v', 225, unsigned char)  //950801
 /* [Sercomm] */
#define VIDIOCISOCSTART         _IOW('v', 226, unsigned char)  //950827
#define VIDIOCISOCSTOP          _IOW('v', 227, unsigned char)
 /* [Sercomm] */
#define VIDIOCGETEXPOSURELEVEL  _IOW('v', 228, unsigned char)  //950903
#define VIDIOCSETEXPOSURELEVEL  _IOW('v', 229, unsigned char)

// 'v', 230, is Get Sensor ID
// 'v', 231, is Set AE

 /* [Sercomm] */
//;new_insert
#define VIDIOCGETSHARPNESS		_IOW('v', 240, int*)  //950929 JJ-
#define VIDIOCSETSHARPNESS		_IOW('v', 241, int)
#define VIDIOCGETGPIO			_IOW('v', 242, struct gpio_status*)  //950929 JJ-
#define VIDIOCSETGPIO			_IOW('v', 243, struct gpio_status*)


#define VIDIOCGETEXPOSURE_EX       _IOW('v', 244, unsigned char)
#define VIDIOCSETEXPOSURE_EX       _IOW('v', 245, unsigned char)

#define VIDIOCGETIRIS_MAX       _IOW('v', 246, unsigned char)
#define VIDIOCGETIRIS_EX       _IOW('v', 247, unsigned char)
#define VIDIOCSETIRIS_EX       _IOW('v', 248, unsigned char)

#define VIDIOCOPENIRIS			_IOW('v', 249, unsigned char)
#define VIDIOCCLOSEIRIS			_IOW('v', 250, unsigned char)
#define VIDIOCIRISDIAGNOSIS		_IOW('v', 251, unsigned char)

#endif
