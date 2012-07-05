#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#include "globals.h"

#include "conf_sec.h"
#include "audio_bulk_ops.h"

#include "audio_shm.h"

int speaker_init();								// initialize the speaker
void speaker_teardown();						// teardown the speaker
static void *audio_thread_func(void *prm);		// thread function

int flow_control(int size);						// flow control of the dropped frame 
