#include <memory.h>
#include <stdio.h>

#include "paDecodeOutStream.h"


/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paStaticOutputCallback(const void*                    inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags,
                          void*                           userData)
{
    paDecodeOutStream *data = (paDecodeOutStream*)userData;
    
    /* call the paDecodeOutStream object counterpart */
    return data->paOutputCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

paDecodeOutStream::paDecodeOutStream(/* args */)
{
}

paDecodeOutStream::~paDecodeOutStream()
{
}

int paDecodeOutStream::InitPaOutputData(PaSampleFormat sampleFormat, long bufferElements, unsigned int outputChannels, unsigned int rate)
{
    int err;

    long writeDataBufElementCount = bufferElements;
    long writeSampleSize = Pa_GetSampleSize(sampleFormat);
    this->rBufToRTData = ALLIGNEDMALLOC(writeSampleSize * writeDataBufElementCount);
    PaUtil_InitializeRingBuffer(&this->rBufToRT, writeSampleSize, writeDataBufElementCount, this->rBufToRTData);
    this->frameSizeBytes = writeSampleSize;
    this->channels = outputChannels;

    if (rate == 48000) {
        this->decoder = opus_decoder_create(rate, outputChannels, &err);
        if (this->decoder == NULL) {
            fprintf(stderr, "opus_decoder_create: %s\n",
                opus_strerror(err));
            return -1;
        }
    }

    return 0;
}
int paDecodeOutStream::paOutputCallback( 
                            const void*                    inputBuffer,
                            void*                           outputBuffer,
                            unsigned long                   framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags           statusFlags)
{
    (void) inputBuffer; /* Prevent "unused variable" warnings. */

    /* Reset output data first */
    memset(outputBuffer, 0, framesPerBuffer * this->channels * this->frameSizeBytes);

    unsigned long availableInReadBuffer = (unsigned long)PaUtil_GetRingBufferReadAvailable(&this->rBufToRT);
    ring_buffer_size_t actualFramesRead = 0;

    if (availableInReadBuffer >= framesPerBuffer) {
        actualFramesRead = PaUtil_ReadRingBuffer(&this->rBufToRT, outputBuffer, framesPerBuffer);
        
        // if actualFramesRead < framesPerBuffer then we read not enough data
    }

    // if framesPerBuffer > availableInReadBuffer we have a buffer underrun
    return paContinue;
}

int paDecodeOutStream::ProtoOpenOutputStream(
		unsigned int rate, unsigned int channels, unsigned int device, PaSampleFormat sampleFormat, int framesPerBuffer)
{
	PaStreamParameters outputParameters;
    outputParameters.device = device;
	outputParameters.channelCount = channels;
	outputParameters.sampleFormat = sampleFormat;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err;
	err = Pa_OpenStream(	&this->stream,
							NULL,
							&outputParameters,
							rate,
							framesPerBuffer,
							paNoFlag,
							paStaticOutputCallback,
							this
	);
	CHK("ProtoOpenOutputStream", err);

	printf("ProtoOpenOutputStream information:\n");
	log_pa_stream_info(this->stream, &outputParameters);
	return 0;
}

int paDecodeOutStream::GetRingBufferWriteAvailable()
{
    return PaUtil_GetRingBufferWriteAvailable(&this->rBufToRT);
}

int paDecodeOutStream::opusDecodeFloat(
            const unsigned char *data,
            opus_int32 len,
            float *pcm,
            int frame_size,
            int decode_fec
        )
{
    return opus_decode_float(this->decoder,
                                                    data,
                                                    len,
                                                    pcm,
                                                    frame_size,
                                                    0); // request in-band forward error correction
                                                        // TODO: this is 1 in rx when no packet was received/lost?
}

int paDecodeOutStream::OpusDecode(
            const unsigned char *data,
            opus_int32 len,
            opus_int16 *pcm,
            int frame_size,
            int decode_fec
        )
{
    return opus_decode(this->decoder,
                                                    data,
                                                    len,
                                                    pcm,
                                                    frame_size,
                                                    0); // request in-band forward error correction
                                                        // TODO: this is 1 in rx when no packet was received/lost?
}

ring_buffer_size_t paDecodeOutStream::WriteRingBuffer( const void *data, ring_buffer_size_t elementCount )
{
    return PaUtil_WriteRingBuffer(&this->rBufToRT, data, elementCount);
}

PaError paDecodeOutStream::StartStream()
{
    return Pa_StartStream(this->stream);
}

PaError paDecodeOutStream::IsStreamActive()
{
    return Pa_IsStreamActive(this->stream);
}

PaError paDecodeOutStream::StopStream()
{
    return Pa_StopStream(this->stream);
}