#ifndef _PA_DECODE_OUT_STREAM
#define _PA_DECODE_OUT_STREAM

#include "paStreamCommon.h"

typedef struct
{
    /* Ring buffer (FIFO) for "communicating" towards audio callback */
    PaUtilRingBuffer    rBufToRT;
    void*               rBufToRTData;
    int                 frameSizeBytes;
    int                 channels;
    OpusDecoder*        decoder;
} paOutputData;

int ProtoOpenOutputStream(PaStream **stream,
		unsigned int rate, unsigned int channels, unsigned int device, paOutputData *pWriteData, PaSampleFormat sampleFormat, int framesPerBuffer);

paOutputData* InitPaOutputData(PaSampleFormat sampleFormat, long bufferElements, unsigned int outputChannels, unsigned int rate);

#endif