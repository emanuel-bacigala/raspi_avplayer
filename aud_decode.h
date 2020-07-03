#ifndef _AUD_DECODE_
#define _AUD_DECODE_

#define BUFFER_SIZE_SAMPLES     8192
#define CTTW_SLEEP_TIME         10
#define MIN_LATENCY_TIME        20


void* handleAudioThread(void *params);


#endif
