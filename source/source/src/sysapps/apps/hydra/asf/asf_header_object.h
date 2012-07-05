#ifndef ASF_HEADER_OBJECT_H
#define ASF_HEADER_OBJECT_H
#include <sys/uio.h>
//#include <IMG_conf.h>
#define AUDIO_ENCODER_TYPE_G711_ALAW  	0x06
#define AUDIO_ENCODER_TYPE_G711_MULAW 	0x07
#define AUDIO_ENCODER_TYPE_G726 		0x45
struct HDL_HeaderObjectConf
{
	unsigned int is_mp9;

	unsigned short VideoResolutionWidth;
	unsigned short VideoResolutionHeight;
	unsigned int VideoBitrate;
	unsigned char* VideoMPEG4Header;

	unsigned int AudioEnable;
	unsigned short AudioNumChannels; 
	unsigned short AudioSamplesPerSec; 
	unsigned short AudioBitsPerSample; 
	unsigned int AudioEncoderType;

	unsigned char* EmptyBuf;
	unsigned int EmptyBufSize;

	unsigned int PacketSize;
};

int ASFSetupHeaderObjectHeader(struct HDL_HeaderObjectConf conf,
	struct iovec* vector, int* count);

#endif

