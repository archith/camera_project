typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

//int g726_decode_frame(unsigned short *dst, int buf_size, uint8_t *data);

void set_signal_mask();
static void *audio_thread_func(void *prm);
