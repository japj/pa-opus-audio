#ifndef _PA_ENCODE_IN_STREAM
#define _PA_ENCODE_IN_STREAM

#include "paStreamCommon.h"

typedef struct
{ 
    /* Ring buffer (FIFO) for "communicating" from audio callback */
    PaUtilRingBuffer    rBufFromRT;
    void*               rBufFromRTData;
    int                 frameSizeBytes;
    int                 channels;
    OpusEncoder*        encoder;
} paInputData;

int ProtoOpenInputStream(PaStream **stream,
        unsigned int rate, unsigned int channels, unsigned int device, paInputData *pReadData, PaSampleFormat sampleFormat, int framesPerBuffer);

paInputData* InitPaInputData(PaSampleFormat sampleFormat, long bufferElements, unsigned int inputChannels, unsigned int rate);

#endif