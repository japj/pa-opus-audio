#include <memory.h>
#include <stdio.h>

#include "paDecodeOutStream.h"


/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paOutputCallback(const void*                    inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags,
                          void*                           userData)
{
    int i;
    paOutputData *data = (paOutputData*)userData;
    (void) inputBuffer; /* Prevent "unused variable" warnings. */

    /* Reset output data first */
    memset(outputBuffer, 0, framesPerBuffer * data->channels * data->frameSizeBytes);

    ring_buffer_size_t availableInReadBuffer = PaUtil_GetRingBufferReadAvailable(&data->rBufToRT);
    ring_buffer_size_t actualFramesRead = 0;

    if (availableInReadBuffer >= framesPerBuffer) {
        actualFramesRead = PaUtil_ReadRingBuffer(&data->rBufToRT, outputBuffer, framesPerBuffer);
        
        // if actualFramesRead < framesPerBuffer then we read not enough data
    }

    // if framesPerBuffer > availableInReadBuffer we have a buffer underrun
    return paContinue;
}

int ProtoOpenOutputStream(PaStream **stream,
		unsigned int rate, unsigned int channels, unsigned int device, paOutputData *pWriteData, PaSampleFormat sampleFormat, int framesPerBuffer)
{
	PaStreamParameters outputParameters;
    outputParameters.device = device;//Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = channels;
	outputParameters.sampleFormat = sampleFormat;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err;
	err = Pa_OpenStream(	stream,
							NULL,
							&outputParameters,
							rate,
							framesPerBuffer,
							paNoFlag,
							paOutputCallback,
							pWriteData
	);
	CHK("ProtoOpenOutputStream", err);

	printf("ProtoOpenOutputStream information:\n");
	log_pa_stream_info(*stream, &outputParameters);
	return 0;
}

paOutputData* InitPaOutputData(PaSampleFormat sampleFormat, long bufferElements, unsigned int outputChannels, unsigned int rate)
{
    int err;
    paOutputData *od = (paOutputData *)ALLIGNEDMALLOC(sizeof(paOutputData));
    memset(od, 0, sizeof(paOutputData));

    long writeDataBufElementCount = bufferElements;
    long writeSampleSize = Pa_GetSampleSize(sampleFormat);
    od->rBufToRTData = ALLIGNEDMALLOC(writeSampleSize * writeDataBufElementCount);
    PaUtil_InitializeRingBuffer(&od->rBufToRT, writeSampleSize, writeDataBufElementCount, od->rBufToRTData);
    od->frameSizeBytes = writeSampleSize;
    od->channels = outputChannels;

    if (rate == 48000) {
        od->decoder = opus_decoder_create(rate, outputChannels, &err);
        if (od->decoder == NULL) {
            fprintf(stderr, "opus_decoder_create: %s\n",
                opus_strerror(err));
            return NULL;
        }
    }

    return od;
}