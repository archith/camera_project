#define AUDIO_SLOT_NUM 480

#define AUDIO_LOCK_STATE_IDLE		0
#define AUDIO_LOCK_STATE_WAITING	1
#define AUDIO_LOCK_STATE_LOCKING	2
#define AUDIO_LOCK_STATE_DONE		3


int init_ac97_device(void);
int read_ac97_device(void);
int close_ac97_device(void);

int init_g726_encoder(void);

int g726_init(void);
int g726_encode_frame(uint8_t *dst, int buf_size, void *data);
int g726_decode_frame(int16_t *dst, int buf_size, uint8_t *data);

int g711_encode_frame(uint8_t *dst, int buf_size, uint16_t *data);
int g711_decode_frame(uint16_t *dst, int buf_size, uint8_t *data);

int g711_alaw_encode_frame(int8_t *dst, int buf_size, int16_t *data);
int g711_ulaw_encode_frame(int8_t *dst, int buf_size, int16_t *data);

void process_rtp_download(char *data, int len, int type);

