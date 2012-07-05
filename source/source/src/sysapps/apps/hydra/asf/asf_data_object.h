#ifndef ASF_DATA_OBJECT_H
#define ASF_DATA_OBJECT_H
#include <sys/uio.h>


struct DOL_DataObjectConf
{
	short IsIFrame;  			// 1 - I frame, 0 - P frame
	short IsAudio;  			// 1 - audio, 0 - video

	unsigned long FrameTime;	//time stamp
	unsigned int FrameCount;
	unsigned int FrameSize;	
	unsigned char* FrameData;

	unsigned char* EmptyBuf;	//for storing headers
	unsigned int EmptyBufSize;

	unsigned int PacketSize;	//each packet size
	unsigned char* ZeroArea;

	unsigned int SCPaddingSize;
	unsigned char* SCPaddingData;
	
	unsigned int MotionDataSize;
	unsigned char* MotionData;

	unsigned int InputDataSize;
	unsigned char* InputData;

	unsigned int duration;
	unsigned long BaseTime;
	unsigned long AudioBaseTime;
	unsigned long VideoBaseTime;
	unsigned long PrevSendTime;
};
int ASFSetupDataObjectHeader(struct DOL_DataObjectConf* conf,
	struct iovec* vector, int* count);

int ASFSetupPackets(struct DOL_DataObjectConf* conf, struct iovec* vector,
	int* count);
#endif
