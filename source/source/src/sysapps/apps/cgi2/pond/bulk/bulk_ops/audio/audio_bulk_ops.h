/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _AUDIO_BILK_OPS_H_
#define  _AUDIO_BILK_OPS_H_

#define AUDIO_BULK_NAME	"AUDIO"

#define ODO_OK		0
#define ODO_ERROR	-1

#define ODO_ENCODED_TYPE_G711_ALAW		0
#define ODO_ENCODED_TYPE_G711_ULAW		1
#define ODO_ENCODED_TYPE_G726_2KBPS		2
#define ODO_ENCODED_TYPE_G726_3KBPS		3
#define ODO_ENCODED_TYPE_G726_4KBPS		4
#define ODO_ENCODED_TYPE_G726_5KBPS		5
#define ODO_ENCODED_TYPE_G729S			6

#define MIC_IN_DISABLE		0
#define MIC_IN_ENABLE		1

#define MIC_IN_MIN_VOLMUE	0
#define MIC_IN_MAX_VOLMUE	16

#define MIC_IN_MIN_TYPE		0
#define MIC_IN_MAX_TYPE		6

#define SPEAKER_OUT_DISABLE		0
#define SPEAKER_OUT_ENABLE		1

#define SPEAKER_OUT_MIN_VOLMUE	0
#define SPEAKER_OUT_MAX_VOLMUE	16

#define SPEAKER_OUT_MIN_TYPE	0
#define SPEAKER_OUT_MAX_TYPE	6

#define AUDIO_MODE_DISABLE		0
#define AUDIO_MODE_ENABLE		1

#define OPERATION_MODE_MIN		0
#define OPERATION_MODE_MAX		3	


struct AUDIO_DS
{
	int mic_enable;			// disable: 0, enable: 1
	int mic_sample_rate;	// default: 8000
	int mic_sample_size;	// default: 16
	int mic_channel;		// default: 1
	int mic_volmue;			// default: 10
	int mic_encoded_type;	// reference to the ODO_ENCODED_TYPE*
 
	int speaker_enable;		// disable: 0, enable: 1
	int speaker_sample_rate;	// default: 8000
	int speaker_sample_size;	// default: 2 (encoded sample size)
	int speaker_channel;		// default: 1
	int speaker_volmue;		// default: 10
	int speaker_encoded_type;	// reference to the ODO_ENCODED_TYPE*

	int audio_mode;			// audio feature, 0: disable, 1: enable
	int operation_mode;		// audio mode setting, 0: simple - listen, 1: simple - talk, 2: half duplex, 3: full duplex
};

int AUDIO_BULK_ReadDS(void* ds);
int AUDIO_BULK_CheckDS(void* ds, void* ds_org);
int AUDIO_BULK_WriteDS(void* ds, void* ds_org);
int AUDIO_BULK_RunDS(void* ds, void* ds_org);

#endif
