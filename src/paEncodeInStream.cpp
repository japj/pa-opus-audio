#include <stdio.h>
#include <memory.h>

#include "paEncodeInStream.h"


/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paInputCallback(const void*                    inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags,
                          void*                           userData)
{
    paInputData *data = (paInputData*)userData;
    (void) outputBuffer; /* Prevent "unused variable" warnings. */

    unsigned long availableWriteFramesInRingBuffer = (unsigned long)PaUtil_GetRingBufferWriteAvailable(&data->rBufFromRT);
    ring_buffer_size_t written = 0;

    // for now, we only write to the ring buffer if enough space is available
    if (framesPerBuffer <= availableWriteFramesInRingBuffer)
    {
        written = PaUtil_WriteRingBuffer(&data->rBufFromRT, inputBuffer, framesPerBuffer);
        // check if fully written?
        return paContinue;
    }

    // if we can't write data to ringbuffer, stop recording for now to early detect issues
    // TODO: determine what best to do here
    return paAbort;
}

int ProtoOpenInputStream(PaStream **stream,
        unsigned int rate, unsigned int channels, unsigned int device, paInputData *pReadData, PaSampleFormat sampleFormat, int framesPerBuffer)
{
	PaStreamParameters inputParameters;
    inputParameters.device = device,
	inputParameters.channelCount = channels;
	inputParameters.sampleFormat = sampleFormat;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err;
	err = Pa_OpenStream(	stream,
							&inputParameters,
							NULL,
							rate,
							framesPerBuffer,
							paNoFlag,
							paInputCallback,
							pReadData
	);
	CHK("ProtoOpenInputStream", err);

	printf("ProtoOpenInputStream information:\n");
	log_pa_stream_info(*stream, &inputParameters);
	return 0;
}

paInputData* InitPaInputData(PaSampleFormat sampleFormat, long bufferElements, unsigned int inputChannels, unsigned int rate)
{
    int err;
    paInputData *id = (paInputData *)ALLIGNEDMALLOC(sizeof(paInputData));
    memset(id, 0, sizeof(paInputData));   

    long readDataBufElementCount = bufferElements;
    long readSampleSize = Pa_GetSampleSize(sampleFormat);
    id->rBufFromRTData = ALLIGNEDMALLOC(readSampleSize * readDataBufElementCount);
    PaUtil_InitializeRingBuffer(&id->rBufFromRT, readSampleSize, readDataBufElementCount, id->rBufFromRTData);
    id->frameSizeBytes = readSampleSize;
    id->channels = inputChannels;

    if (rate == 48000) {
        id->encoder = opus_encoder_create(rate, inputChannels, OPUS_APPLICATION_AUDIO,
				&err);
        if (id->encoder == NULL) {
            fprintf(stderr, "opus_encoder_create: %s\n",
                opus_strerror(err));
            return NULL;
        }
    }

    return id;
}