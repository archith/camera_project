#ifndef __CRC_H__
#define __CRC_H__

unsigned short crc16_update (unsigned char octet, unsigned short crc);
unsigned short crc16_compute (unsigned char *data, unsigned int nbyte);

unsigned int crc32_update (unsigned char octet, unsigned int crc);
unsigned int crc32_compute (unsigned char *data, unsigned int nbyte);

#endif // __CRC_H__
