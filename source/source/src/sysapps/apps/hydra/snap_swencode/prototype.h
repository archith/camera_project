#ifndef PROTOTYPE_H
#define PROTOTYPE_H

void initialization (JPEG_ENCODER_STRUCTURE *, UINT32, UINT32, UINT32);

UINT16 DSP_Division (UINT32, UINT32);
void initialize_quantization_tables (UINT32);

UINT8* write_markers (UINT8 *, UINT32, UINT32, UINT32);

UINT8* encode_image (UINT8 *,UINT8 *, UINT32, UINT32, UINT32, UINT32);

void read_400_format (JPEG_ENCODER_STRUCTURE *, UINT8 *);
void read_420_format (JPEG_ENCODER_STRUCTURE *, UINT8 *);
void read_422_format (JPEG_ENCODER_STRUCTURE *, UINT8 *);
void read_444_format (JPEG_ENCODER_STRUCTURE *, UINT8 *);
void RGB_2_444 (UINT8 *, UINT8 *, UINT32, UINT32);

UINT8* encodeMCU (JPEG_ENCODER_STRUCTURE *, UINT32, UINT8 *);

void levelshift (INT16 *);
void DCT (INT16 *);
void quantization (INT16 *, UINT16 *);
UINT8* huffman (JPEG_ENCODER_STRUCTURE *, UINT16, UINT8 *);

UINT8* close_bitstream (UINT8 *);
//void* jemalloc(size_t);
//void* jefree(void);
#endif
