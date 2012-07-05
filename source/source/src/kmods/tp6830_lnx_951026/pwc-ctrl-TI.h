#ifdef TP6830_TICCD

// ----------------------------------

// "pwc-ctrl-TI.h"

// #ifdef TP6830_TICCD

// #endif

// ---------------------------------- 


///////////////////////////////////////

// CCD Exposure Structure

///////////////////////////////////////


typedef struct _CCD_EXPOSURE_STRUCT
{

 BYTE TI_Exposure;     	// Exposure raw id


 BYTE TI_Gain;  	// Gain data


 BYTE FrameRate;   	// Frame rate


 BYTE AWB_GainWei;   	// Gain weight


 BYTE PrevExpoOffset;   // Previous Exposure offset


 BYTE NextExpoOffset;   // Next Exposure offset


} CCD_EXPOSURE_STRUCT;//, *pCCD_EXPOSURE_STRUCT;

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

#define	TI_EXPOSURE		0
#define TI_GAIN			1
#define FRAME_RATE		2
#define AWB_GAINWEI		3
#define PREV_EXPO_OFST	4
#define NEXT_EXPO_OFST	5

#define IRIS_CLOSE		6
#define IRIS_OPEN		7

//; Data_byte Number
#define BN_EXPO	1
#define BN_GAIN	1
#define BN_FR	1
#define BN_AWBG	1
#define BN_OFST	2

#define BN_ICLOSE	1
#define BN_IOPEN	1
//;

#define DAC1_LSB	0x0a
#define DAC1_MSB	0x0b
#define DAC2_LSB	0x0c
#define DAC2_MSB	0x0d

#define DAC_CONTROL_LSB			DAC1_LSB  // DAC2_LSB
#define DAC_CONTROL_MSB			DAC1_MSB  // DAC2_MSB

extern int Frequency;


BYTE CCD_EXPOSURE_TABLE50[][BN_EXPO+BN_GAIN+BN_FR+BN_AWBG+BN_OFST] = {
	{0x00, 0x00,  0, 00,  00, 00},  //0

	{0x00, 0x02, 30, 45,  10, 16},  //1
	{0x00, 0x02, 30, 45,  10, 16},  //2
	{0x00, 0x02, 30, 50,  10, 16},  //3
	{0x00, 0x02, 30, 50,  10, 16},  //4
	{0x00, 0x02, 30, 55,  10, 16},  //5
	{0x00, 0x02, 30, 55,  10, 16},  //6
	{0x00, 0x02, 30, 60,  10, 16},  //7
	{0x00, 0x02, 30, 65,  10, 16},  //8
	{0x00, 0x02, 30, 65,  10, 16},  //9
	{0x00, 0x02, 30, 70,  10, 16},  //10
	{0x00, 0x02, 30, 70,  10, 16},  //11
	{0x00, 0x02, 30, 75,  10, 16},  //12
	{0x00, 0x02, 30, 75,  10, 16},  //13
	{0x00, 0x02, 30, 80,  10, 16},  //14
	{0x00, 0x02, 30, 80,  10, 16},  //15
	{0x00, 0x02, 30, 85,  10, 16},  //16
	{0x00, 0x02, 30, 85,  10, 16},  //17
	{0x00, 0x02, 30, 90,  10, 16},  //18
	{0x00, 0x02, 30, 90,  10, 16},  //19
	{0x00, 0x02, 30, 95,  10, 16},  //20
	{0x00, 0x02, 30, 100, 10, 16},  //21
	{0x00, 0x03, 30, 95,  10, 16},  //22
	{0x00, 0x03, 30, 100, 10, 16},  //23
	{0x00, 0x04, 30, 100, 10, 16},  //24
	{0x00, 0x04, 30, 105, 10, 16},  //25
	{0x00, 0x05, 30, 95,  10, 16},  //26
	{0x00, 0x05, 30, 100, 10, 16},  //27
	{0x00, 0x05, 30, 105, 10, 16},  //28
	{0x00, 0x06, 30, 95,  10, 16},  //29
	{0x00, 0x06, 30, 100, 10, 16},  //30
	{0x00, 0x06, 30, 105, 10, 16},  //31
	{0x00, 0x07, 30, 100, 10, 16},  //32
	{0x00, 0x07, 30, 100, 10, 16},  //33
	{0x00, 0x07, 30, 105, 10, 16},  //34
	{0x00, 0x07, 30, 105, 10, 16},  //35
	{0x00, 0x07, 30, 110, 10, 16},  //36
	{0x00, 0x07, 30, 110, 10, 16},  //37
	{0x00, 0x07, 30, 115, 10, 16},  //38
	{0x00, 0x07, 30, 115, 10, 16},  //39
	{0x00, 0x07, 30, 120, 10, 16},  //40
	{0x00, 0x07, 30, 120, 10, 16},  //41
	{0x00, 0x07, 30, 125, 10, 16},  //42
	{0x00, 0x07, 30, 125, 10, 16},  //43

	{0x01, 0x01, 30, 80,  10, 16},  //44
	{0x01, 0x01, 30, 80,  10, 16},  //45
	{0x01, 0x01, 30, 85,  10, 16},  //46
	{0x01, 0x01, 30, 85,  10, 16},  //47
	{0x01, 0x01, 30, 90,  10, 16},  //48
	{0x01, 0x01, 30, 90,  10, 16},  //49
	{0x01, 0x01, 30, 95,  10, 16},  //50
	{0x01, 0x01, 30, 95,  10, 16},  //51
	{0x01, 0x01, 30, 100, 10, 16},  //52
	{0x01, 0x01, 30, 100, 10, 16},  //53
	{0x01, 0x01, 30, 105, 10, 16},  //54
	{0x01, 0x01, 30, 105, 10, 16},  //55
	{0x01, 0x02, 30, 95,  10, 16},  //56
	{0x01, 0x02, 30, 100, 10, 16},  //57
	{0x01, 0x02, 30, 105, 10, 16},  //58
	{0x01, 0x03, 30, 100, 10, 16},  //59
	{0x01, 0x03, 30, 105, 10, 16},  //60
	{0x01, 0x04, 30, 95,  10, 16},  //61
	{0x01, 0x04, 30, 95,  10, 16},  //62
	{0x01, 0x04, 30, 100, 10, 16},  //63
	{0x01, 0x04, 30, 100, 10, 16},  //64
	{0x01, 0x04, 30, 105, 10, 16},  //65
	{0x01, 0x04, 30, 105, 10, 16},  //66
	{0x01, 0x04, 30, 110, 10, 16},  //67
	{0x01, 0x04, 30, 110, 10, 16},  //68

	{0x02, 0x03, 30, 95,  10, 16},  //69
	{0x02, 0x03, 30, 95,  10, 16},  //70
	{0x02, 0x03, 30, 100, 10, 16},  //71
	{0x02, 0x03, 30, 100, 10, 16},  //72
	{0x02, 0x03, 30, 105, 10, 16},  //73
	{0x02, 0x03, 30, 105, 10, 16},  //74
	{0x02, 0x04, 30, 100, 10, 16},  //75
	{0x02, 0x04, 30, 105, 10, 16},  //76
	{0x02, 0x05, 30, 100, 10, 16},  //77
	{0x02, 0x05, 30, 105, 10, 16},  //78
	{0x02, 0x06, 30, 100, 10, 16},  //79
	{0x02, 0x06, 30, 100, 10, 16},  //80
	{0x02, 0x06, 30, 105, 10, 16},  //81
	{0x02, 0x06, 30, 105, 10, 16},  //82
	{0x02, 0x06, 30, 110, 10, 16},  //83
	{0x02, 0x06, 30, 110, 10, 16},  //84

	{0x03, 0x05, 30, 100, 10, 16},  //85
	{0x03, 0x05, 30, 100, 10, 16},  //86
	{0x03, 0x05, 30, 105, 10, 16},  //87
	{0x03, 0x05, 30, 105, 10, 16},  //88
	{0x03, 0x06, 30, 100, 10, 16},  //89
	{0x03, 0x06, 30, 105, 10, 16},  //90
	{0x03, 0x07, 30, 100, 10, 16},  //91
	{0x03, 0x07, 30, 105, 10, 16},  //92
	{0x03, 0x07, 30, 110, 10, 16},  //93
	{0x03, 0x07, 30, 115, 10, 16},  //94

	{0x03, 0x07, 25, 100, 10, 16},  //95
	{0x03, 0x07, 25, 105, 10, 16},  //96
	{0x03, 0x07, 25, 110, 10, 16},  //97
	{0x03, 0x07, 25, 115, 10, 16},  //98
	{0x03, 0x07, 20, 100, 10, 16},  //99
	{0x03, 0x07, 20, 105, 10, 16},  //100
	{0x03, 0x07, 20, 110, 10, 16},  //101
	{0x03, 0x07, 20, 115, 10, 16},  //102
	{0x03, 0x07, 20, 120, 10, 16},  //103
	{0x03, 0x07, 20, 127, 10, 16},  //104
	{0x03, 0x07, 15, 100, 10, 16},  //105
	{0x03, 0x07, 15, 105, 10, 16},  //106
	{0x03, 0x07, 15, 110, 10, 16},  //107

	{0x03, 0x07, 15, 110, 10, 16},  //108
	{0x03, 0x07, 15, 115, 10, 16},  //109
	{0x03, 0x07, 15, 120, 10, 16},  //110
	{0x03, 0x07, 15, 125, 10, 16},  //111
	{0x03, 0x07, 15, 130, 10, 16},  //112

	{0x03, 0x07, 13, 100, 10, 16},  //113
	{0x03, 0x07, 13, 105, 10, 16},  //114
	{0x03, 0x07, 13, 110, 10, 16},  //115
	{0x03, 0x07, 13, 115, 10, 16},  //116

	{0x03, 0x07, 10, 100, 10, 16},  //117
	{0x03, 0x07, 10, 105, 10, 16},  //118
	{0x03, 0x07, 10, 110, 10, 16},  //119
	{0x03, 0x07, 10, 115, 10, 16},  //120
	{0x03, 0x07, 10, 120, 10, 16},  //121
	{0x03, 0x07, 10, 125, 10, 16},  //122
	{0x03, 0x07, 10, 130, 10, 16},  //123
	{0x03, 0x07, 10, 135, 10, 16},  //124
	{0x03, 0x07, 10, 140, 10, 16},  //125
	{0x03, 0x07, 10, 145, 10, 16},  //126
	{0x03, 0x07, 10, 150, 10, 16},  //127
	{0x03, 0x07, 10, 155, 10, 16},  //128
	{0x03, 0x07, 10, 160, 10, 16},  //129
	{0x03, 0x07, 10, 165, 10, 16},  //130
	{0x03, 0x07, 10, 170, 10, 16},  //131
	{0x03, 0x07, 10, 175, 10, 16},  //132
	{0x03, 0x07, 10, 180, 10, 16},  //133
	{0x03, 0x07, 10, 185, 10, 16},  //134
	{0x03, 0x07, 10, 190, 10, 16},  //135
	{0x03, 0x07, 10, 195, 10, 16},  //136
	{0x03, 0x07, 10, 200, 10, 16},  //137
};

BYTE CCD_EXPOSURE_TABLE60[][BN_EXPO+BN_GAIN+BN_FR+BN_AWBG+BN_OFST] = {
	{0x00, 0x00,  0, 00,  00, 00},  //0

	{0x00, 0x02, 30, 45,  10, 16},  //1
	{0x00, 0x02, 30, 45,  10, 16},  //2
	{0x00, 0x02, 30, 50,  10, 16},  //3
	{0x00, 0x02, 30, 50,  10, 16},  //4
	{0x00, 0x02, 30, 55,  10, 16},  //5
	{0x00, 0x02, 30, 55,  10, 16},  //6
	{0x00, 0x02, 30, 60,  10, 16},  //7
	{0x00, 0x02, 30, 65,  10, 16},  //8
	{0x00, 0x02, 30, 65,  10, 16},  //9
	{0x00, 0x02, 30, 70,  10, 16},  //10
	{0x00, 0x02, 30, 70,  10, 16},  //11
	{0x00, 0x02, 30, 75,  10, 16},  //12
	{0x00, 0x02, 30, 75,  10, 16},  //13
	{0x00, 0x02, 30, 80,  10, 16},  //14
	{0x00, 0x02, 30, 80,  10, 16},  //15
	{0x00, 0x02, 30, 85,  10, 16},  //16
	{0x00, 0x02, 30, 85,  10, 16},  //17
	{0x00, 0x02, 30, 90,  10, 16},  //18
	{0x00, 0x02, 30, 90,  10, 16},  //19
	{0x00, 0x02, 30, 95,  10, 16},  //20
	{0x00, 0x02, 30, 100, 10, 16},  //21
	{0x00, 0x03, 30, 95,  10, 16},  //22
	{0x00, 0x03, 30, 100, 10, 16},  //23
	{0x00, 0x04, 30, 100, 10, 16},  //24
	{0x00, 0x04, 30, 105, 10, 16},  //25
	{0x00, 0x05, 30, 95,  10, 16},  //26
	{0x00, 0x05, 30, 100, 10, 16},  //27
	{0x00, 0x05, 30, 105, 10, 16},  //28
	{0x00, 0x06, 30, 95,  10, 16},  //29
	{0x00, 0x06, 30, 100, 10, 16},  //30
	{0x00, 0x06, 30, 105, 10, 16},  //31
	{0x00, 0x07, 30, 100, 10, 16},  //32
	{0x00, 0x07, 30, 100, 10, 16},  //33
	{0x00, 0x07, 30, 105, 10, 16},  //34
	{0x00, 0x07, 30, 105, 10, 16},  //35
	{0x00, 0x07, 30, 110, 10, 16},  //36
	{0x00, 0x07, 30, 110, 10, 16},  //37
	{0x00, 0x07, 30, 115, 10, 16},  //38
	{0x00, 0x07, 30, 115, 10, 16},  //39
	{0x00, 0x07, 30, 120, 10, 16},  //40
	{0x00, 0x07, 30, 120, 10, 16},  //41
	{0x00, 0x07, 30, 125, 10, 16},  //42
	{0x00, 0x07, 30, 125, 10, 16},  //43

	{0x01, 0x01, 30, 80,  10, 16},  //44
	{0x01, 0x01, 30, 80,  10, 16},  //45
	{0x01, 0x01, 30, 85,  10, 16},  //46
	{0x01, 0x01, 30, 85,  10, 16},  //47
	{0x01, 0x01, 30, 90,  10, 16},  //48
	{0x01, 0x01, 30, 90,  10, 16},  //49
	{0x01, 0x01, 30, 95,  10, 16},  //50
	{0x01, 0x01, 30, 95,  10, 16},  //51
	{0x01, 0x01, 30, 100, 10, 16},  //52
	{0x01, 0x01, 30, 100, 10, 16},  //53
	{0x01, 0x01, 30, 105, 10, 16},  //54
	{0x01, 0x01, 30, 105, 10, 16},  //55
	{0x01, 0x02, 30, 95,  10, 16},  //56
	{0x01, 0x02, 30, 100, 10, 16},  //57
	{0x01, 0x02, 30, 105, 10, 16},  //58
	{0x01, 0x03, 30, 100, 10, 16},  //59
	{0x01, 0x03, 30, 105, 10, 16},  //60
	{0x01, 0x04, 30, 95,  10, 16},  //61
	{0x01, 0x04, 30, 95,  10, 16},  //62
	{0x01, 0x04, 30, 100, 10, 16},  //63
	{0x01, 0x04, 30, 100, 10, 16},  //64
	{0x01, 0x04, 30, 105, 10, 16},  //65
	{0x01, 0x04, 30, 105, 10, 16},  //66
	{0x01, 0x04, 30, 110, 10, 16},  //67
	{0x01, 0x04, 30, 110, 10, 16},  //68

	{0x02, 0x03, 30, 95,  10, 16},  //69
	{0x02, 0x03, 30, 95,  10, 16},  //70
	{0x02, 0x03, 30, 100, 10, 16},  //71
	{0x02, 0x03, 30, 100, 10, 16},  //72
	{0x02, 0x03, 30, 105, 10, 16},  //73
	{0x02, 0x03, 30, 105, 10, 16},  //74
	{0x02, 0x04, 30, 100, 10, 16},  //75
	{0x02, 0x04, 30, 105, 10, 16},  //76
	{0x02, 0x05, 30, 100, 10, 16},  //77
	{0x02, 0x05, 30, 105, 10, 16},  //78
	{0x02, 0x06, 30, 100, 10, 16},  //79
	{0x02, 0x06, 30, 100, 10, 16},  //80
	{0x02, 0x06, 30, 105, 10, 16},  //81
	{0x02, 0x06, 30, 105, 10, 16},  //82
	{0x02, 0x06, 30, 110, 10, 16},  //83
	{0x02, 0x06, 30, 110, 10, 16},  //84

	{0x03, 0x05, 30, 100, 10, 16},  //85
	{0x03, 0x05, 30, 100, 10, 16},  //86
	{0x03, 0x05, 30, 105, 10, 16},  //87
	{0x03, 0x05, 30, 105, 10, 16},  //88
	{0x03, 0x06, 30, 100, 10, 16},  //89
	{0x03, 0x06, 30, 105, 10, 16},  //90
	{0x03, 0x07, 30, 100, 10, 16},  //91
	{0x03, 0x07, 30, 105, 10, 16},  //92
	{0x03, 0x07, 30, 110, 10, 16},  //93
	{0x03, 0x07, 30, 115, 10, 16},  //94

	{0x03, 0x07, 25, 100, 10, 16},  //95
	{0x03, 0x07, 25, 105, 10, 16},  //96
	{0x03, 0x07, 25, 110, 10, 16},  //97
	{0x03, 0x07, 25, 115, 10, 16},  //98

	{0x03, 0x07, 20, 100, 10, 16},  //99
	{0x03, 0x07, 20, 105, 10, 16},  //100
	{0x03, 0x07, 20, 110, 10, 16},  //101
	{0x03, 0x07, 20, 115, 10, 16},  //102
	{0x03, 0x07, 20, 120, 10, 16},  //103
	{0x03, 0x07, 20, 127, 10, 16},  //104
	
	{0x03, 0x07, 15, 100, 10, 16},  //105
	{0x03, 0x07, 15, 105, 10, 16},  //106
	{0x03, 0x07, 15, 110, 10, 16},  //107
	{0x03, 0x07, 15, 110, 10, 16},  //108
	{0x03, 0x07, 15, 115, 10, 16},  //109
	{0x03, 0x07, 15, 120, 10, 16},  //110
	{0x03, 0x07, 15, 125, 10, 16},  //111
	{0x03, 0x07, 15, 130, 10, 16},  //112

	{0x03, 0x07, 13, 100, 10, 16},  //113
	{0x03, 0x07, 13, 105, 10, 16},  //114
	{0x03, 0x07, 13, 110, 10, 16},  //115
	{0x03, 0x07, 13, 115, 10, 16},  //116

	{0x03, 0x07, 10, 100, 10, 16},  //117
	{0x03, 0x07, 10, 105, 10, 16},  //118
	{0x03, 0x07, 10, 110, 10, 16},  //119
	{0x03, 0x07, 10, 115, 10, 16},  //120
	{0x03, 0x07, 10, 120, 10, 16},  //121
	{0x03, 0x07, 10, 125, 10, 16},  //122
	{0x03, 0x07, 10, 130, 10, 16},  //123
	{0x03, 0x07, 10, 135, 10, 16},  //124
	{0x03, 0x07, 10, 140, 10, 16},  //125
	{0x03, 0x07, 10, 145, 10, 16},  //126
	{0x03, 0x07, 10, 150, 10, 16},  //127
	{0x03, 0x07, 10, 155, 10, 16},  //128
	{0x03, 0x07, 10, 160, 10, 16},  //129
	{0x03, 0x07, 10, 165, 10, 16},  //130
	{0x03, 0x07, 10, 170, 10, 16},  //131
	{0x03, 0x07, 10, 175, 10, 16},  //132
	{0x03, 0x07, 10, 180, 10, 16},  //133
	{0x03, 0x07, 10, 185, 10, 16},  //134
	{0x03, 0x07, 10, 190, 10, 16},  //135
	{0x03, 0x07, 10, 195, 10, 16},  //136
	{0x03, 0x07, 10, 200, 10, 16},  //137
};


#define TI_25FPS_EXPO_NUM	98
#define TI_20FPS_EXPO_NUM	104
#define TI_15FPS_EXPO_NUM	112
#define TI_13FPS_EXPO_NUM	116

#define TI_MAX_EXPO_50	((sizeof(CCD_EXPOSURE_TABLE50)/sizeof(CCD_EXPOSURE_TABLE50[0]))-1)
#define TI_MAX_EXPO_60	((sizeof(CCD_EXPOSURE_TABLE60)/sizeof(CCD_EXPOSURE_TABLE60[0]))-1)





//;#define IRIS_CLOSE		6 -->0
//;#define IRIS_OPEN		7 -->1

BYTE IRIS_EXPO_TABLE[][BN_ICLOSE+BN_IOPEN] = {
	{01, 01},
	// set iris from 6 , 5 .. 1 to close len 
	// set iris from 1, 2, 3, 4, ..6 to open len
	{60, 35},
	{55, 30},
	{53, 25},
	{50, 20},
	{45, 15},
	{40, 10},	// MAX6. to close,  MIN to open
	{40, 05},	// MAX7.
	{40, 00},	// MAX8.
	{00, 00},	// MAX9.
	// ...
};

#define TI_MAX_IRIS	((sizeof(IRIS_EXPO_TABLE)/sizeof(IRIS_EXPO_TABLE[0]))-1)


/*
  unsigned char TICCD_Exposure60[]={

    0x00,0x00,0x00,0x00,0x00,  // (0.)0x02,0x03,0xb0,0x00,0x00, 

    // [0];TI-Exposure

    //     [1];TI-Gain

    //	       [3];FrameRate

    // 	 	    [4];Rsrv

    // 		     	[5];Rsrv

    //

    }; */ 

void topro_TI_MaxExpo(void)

{

  if (Frequency==50)

  //MaxExposure = 12; //(8+4); 

    MaxExposure =  TI_MAX_EXPO_50;

  else

  //MaxExposure = (7+4+13+4+1);

  //MaxExposure = 38; //(-4+8+7+4+13+4+1+5);

    MaxExposure =  TI_MAX_EXPO_60;


  //;

    printk(" ;TI MaxExposure= %d \n", MaxExposure);

}


//;[#endif]


unsigned char Get_TempExposure(int Exposure) // get[Em0]

{

	//;_TICCD_Exposure50[Em0]

	//;_TICCD_Exposure60[Em0]

	if(Frequency == 50)

		return CCD_EXPOSURE_TABLE50[Exposure][TI_EXPOSURE]; //TICCD_Exposure50[Exposure*5];

	else if(Frequency == 60) 

		return CCD_EXPOSURE_TABLE60[Exposure][TI_EXPOSURE]; //TICCD_Exposure60[Exposure*5];

	return 0;

}


unsigned char Get_TempGain(int Exposure)  // get[Em0+1]

{

	//;_TICCD_Exposure50[Em0+1]

	//;_TICCD_Exposure60[Em0+1]

	if(Frequency == 50) 

		return CCD_EXPOSURE_TABLE50[Exposure][TI_GAIN]; //TICCD_Exposure50[Exposure*5+1];

	else if(Frequency == 60)  

		return CCD_EXPOSURE_TABLE60[Exposure][TI_GAIN]; //TICCD_Exposure60[Exposure*5+1];

	return 0;

} 


BYTE Get_FrameRate(int expo) // The frame rate, that mclk will be setted as.
{

	if(Frequency == 50) 
		return CCD_EXPOSURE_TABLE50[expo][FRAME_RATE];
	else if(Frequency == 60)  
		return CCD_EXPOSURE_TABLE60[expo][FRAME_RATE];
	return 0;
}




BYTE Get_AWBGainWei(int expo)

{ 

	if(Frequency == 50) 

		return CCD_EXPOSURE_TABLE50[expo][AWB_GAINWEI];

	else if(Frequency == 60)  

		return CCD_EXPOSURE_TABLE60[expo][AWB_GAINWEI];

	return 0;

}

BYTE Get_PrevOfst(int expo)

{ 

	if(Frequency == 50) 

		return CCD_EXPOSURE_TABLE50[expo][PREV_EXPO_OFST];

	else if(Frequency == 60)  

		return CCD_EXPOSURE_TABLE60[expo][PREV_EXPO_OFST];

	return 0;

}

BYTE Get_NextOfst(int expo)

{ 

	if(Frequency == 50) 

		return CCD_EXPOSURE_TABLE50[expo][NEXT_EXPO_OFST];

	else if(Frequency == 60)  

		return CCD_EXPOSURE_TABLE60[expo][NEXT_EXPO_OFST];

	return 0;

}


BYTE IrisTableItem(int expo, int nItem){
	int i= 1;
	if (nItem==IRIS_CLOSE)
	{
		while (i<=expo)
		{
			if (i==expo) 
				return IRIS_EXPO_TABLE[i][0];
			//;		  if (IRIS_EXPO_TABLE[i][0]==40) return 40;
			if (i>=TI_MAX_IRIS) 
				return IRIS_EXPO_TABLE[TI_MAX_IRIS][0];
			i++;
		}
		//;return 40;
		return IRIS_EXPO_TABLE[TI_MAX_IRIS][0];
	}

	if (nItem==IRIS_OPEN)
	{
		while (i<=expo)
		{
			if (i==expo) 
				return IRIS_EXPO_TABLE[i][1]; //[IRIS_OPEN-IRIS_CLOSE];
			//;		  if (IRIS_EXPO_TABLE[i][1]==0) return 40;
			if (i>=TI_MAX_IRIS) 
				return IRIS_EXPO_TABLE[TI_MAX_IRIS][1]; //[IRIS_OPEN-IRIS_CLOSE];
			i++;
		}
		//;return 0;
		return IRIS_EXPO_TABLE[TI_MAX_IRIS][1];
	}

	//;return IRIS_EXPO_TABLE[expo][nItem-IRIS_CLOSE];  // CLOSE:0, OPEN:1
	return NULL;
}

BYTE NormalTableItem(int expo, int nItem){
	if(Frequency == 50) 
		return CCD_EXPOSURE_TABLE50[expo][nItem];
	else if(Frequency == 60)  
		return CCD_EXPOSURE_TABLE60[expo][nItem];
}

BYTE Get_TableItem(int expo, int nItem)
{ 
	if (nItem==IRIS_CLOSE || nItem==IRIS_OPEN)
	{
		return IrisTableItem(expo,nItem);
	}
	else
		return NormalTableItem(expo,nItem);
	return 0;
}



long Calc_TempExposure(int Exposure)

{

	long temp4;

	long CCDExposure;

	temp4 = 12;

	if(Frequency == 50) {
		CCDExposure = 0x20c - ( Get_TempExposure(Exposure) * temp4 * 1000000 / (Frequency * 2) - 447) / 780;  // Frequency=50Hz, ­«ºâ
	}

	else if(Frequency == 60) {
		CCDExposure = 0x20c - ( Get_TempExposure(Exposure) * temp4 * 1000000 / (Frequency * 2) - 447) / 780;  // Frequency= 60

	}

	else
		CCDExposure = 0x20c - Exposure * 5;

  return CCDExposure;

}


long Calc_CCDGain(int Exposure)

{

	long CCDGain= Get_TempGain(Exposure);

  return CCDGain << 2;

}


void List_CCDExposure(void)

{

	int Frequency_Old;

	int Exposure_Enum;

	//;long CCDExposure;

	//;unsigned char CCDTemp1,CCDTemp2;


	Frequency_Old= Frequency;

	printk("\n");

	printk("60Hz: H09e H09f H007\n");

	Frequency= 60;

	for (Exposure_Enum=1; Exposure_Enum<=TI_MAX_EXPO_60; Exposure_Enum+=3)

		{

				long CCDExposure= Calc_TempExposure(Exposure_Enum);


				unsigned char	CCDTemp1 = (CCDExposure << 2) & 0xfc;				//low byte

				unsigned char	CCDTemp2 = (CCDExposure >> 4) & 0xfc;				//high byte


				long CCDGain= Calc_CCDGain(Exposure_Enum);

				unsigned char ucdat= Get_TempGain(Exposure_Enum);


				//; printk("List_setexposure(%d):: %02x, %02x, %02x/%02x \n", Exposure_Enum, CCDTemp1,CCDTemp2,CCDGain,ucdat<<2);

				printk("List_setexposure(%d):: %04x (%02x, %02x), %02x (%02x) \n", Exposure_Enum, CCDExposure, CCDTemp1,CCDTemp2, ucdat, CCDGain);

		}


	Frequency= Frequency_Old;

}


//; SIF


#define PRINTK_SIF(ud)		printk("[Ckeck]: 0x11, 0x%02x \n", topro_read_reg(ud, 0x11))


//

// return: 0 - No Error

//         1 - Error 

int WAIT_SIF(struct usb_device *ud)

{

  int k,i;

  unsigned char RegData2;


  k= 50;

  while (k--)

  {

	for(i=0;i<100;i++){  //Waiting for Sif_Control be reseted;

	RegData2 = topro_read_reg(ud, SIF_CONTROL);

	if(RegData2 == 0x00)

		return 0;

	}

	udelay(1);

  }

  return 1;

}



#endif
