#ifndef _QOS_API_H_
#define _QOS_API_H_

#define QOS_THIS_IS_AUDIO_STREAM 	0
#define QOS_THIS_IS_VIDEO_STREAM 	1
#define QOS_THIS_IS_AV_STREAM 		2
int QOSSetDSCPValue(int fd, int stream_type);

#endif
