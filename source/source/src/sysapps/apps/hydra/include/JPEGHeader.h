#ifndef __JPEG_HEADER_H__
#define __JPEG_HEADER_H__

#define JPEG_HEADER_SIZE	(JPEG_SOI_SIZE+JPEG_DQT_SIZE+JPEG_DHT_SIZE+JPEG_SOF_SIZE+JPEG_SOS_SIZE)

#define JPEG_SOI_SIZE		2
#define JPEG_DQT_SIZE		134
#define JPEG_DHT_SIZE		420
#define JPEG_SOF_SIZE		19
#define JPEG_SOS_SIZE		14
#define JPEG_EOI_SIZE		2




int GetJPEGHeader(unsigned char* header, int size, int w, int h, unsigned char *QLuma, unsigned char *QChroma);
int GetJpegSOI(unsigned char* buf, size_t size);
int GetJpegDQT(unsigned char* buf, size_t size, unsigned char *QLuma, unsigned char *QChroma);
int GetJpegDHT(unsigned char* buf, size_t size);
int GetJpegSOF(unsigned char* buf, size_t size, unsigned int w, unsigned int h);
int GetJpegSOS(unsigned char* buf, size_t size);
int GetJpegEOI(unsigned char* buf, size_t size);

#endif
