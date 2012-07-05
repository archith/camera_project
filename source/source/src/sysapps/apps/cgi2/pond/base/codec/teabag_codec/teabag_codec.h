#ifndef TEABAG_CODEC_H
#define TEABAG_CODEC_H

int teabag_encoder(char* src, int srclen, char* dst, int* dstlen);
int teabag_decoder(char* src, int srclen, char* dst, int* dstlen);

#endif
