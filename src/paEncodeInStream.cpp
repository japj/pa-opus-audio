#include <stdio.h>
#include <memory.h>
#include "paEncodeInStream.h"


/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paStaticInputCallback(const void*                    inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags,
                          void*                           userData)
{
    paEncodeInStream *data = (paEncodeInStream*)userData;
    
    /* call the paEncodeInStream object counterpart */
    
    return data->paInputCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

paEncodeInStream::paEncodeInStream(/* args */) 
{
    sampleRate                      = 48000;    // opus only allows certain sample rates for encoding
    sampleFormat                    = paInt16;  //paFloat32 or paInt16;
    channels                        = 1;        //?2; // needs to be 1 if you want to output recorded audio directly without opus enc/dec in order to get same amount of data/structure mono/stereo
                                                // 2 gives garbled sound?
    bufferElements                  = 4096;     // TODO: calculate optimal ringbuffer size
                                                // note opusDecodeBuffer is opusMaxFrameSize * sizeof sampleFormatInBytes
    paCallbackFramesPerBuffer       = 64;       /* since opus decodes 120 frames, this is closests to how our latency is going to be
                                                // frames per buffer for OS Audio buffer*/
    opusMaxFrameSize                = 120;      // 2.5ms@48kHz number of samples per channel in the input signal
    
    //opusMaxFrameSize                = 2880;     // 2280 is sample buffer size for decoding at at 48kHz with 60ms
						                        // for better analysis of the audio I am sending with 60ms from opusrtp
}

paEncodeInStream::~paEncodeInStream()
{
    // TODO: cleanup all memory
}

int paEncodeInStream::InitPaInputData()
{
    int err; 

    long readDataBufElementCount = this->bufferElements;
    long readSampleSize = Pa_GetSampleSize(this->sampleFormat);
    long bufferSize = readSampleSize * readDataBufElementCount;
    this->rBufFromRTData = ALLIGNEDMALLOC(bufferSize);
    PaUtil_InitializeRingBuffer(&this->rBufFromRT, readSampleSize, readDataBufElementCount, this->rBufFromRTData);
    this->frameSizeBytes = readSampleSize;

    if (this->sampleRate == 48000) {
        this->encoder = opus_encoder_create(this->sampleRate, this->channels, OPUS_APPLICATION_AUDIO,
				&err);
        if (this->encoder == NULL) {
            fprintf(stderr, "opus_encoder_create: %s\n",
                opus_strerror(err));
            return -1;
        }
    }

    this->opusEncodeBuffer = ALLIGNEDMALLOC(bufferSize);

    return 0;
}

int paEncodeInStream::paInputCallback(const void*                    inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags)
{
    (void) outputBuffer; /* Prevent "unused variable" warnings. */

    unsigned long availableWriteFramesInRingBuffer = (unsigned long)PaUtil_GetRingBufferWriteAvailable(&this->rBufFromRT);
    ring_buffer_size_t written = 0;

    // for now, we only write to the ring buffer if enough space is available
    if (framesPerBuffer <= availableWriteFramesInRingBuffer)
    {
        written = PaUtil_WriteRingBuffer(&this->rBufFromRT, inputBuffer, framesPerBuffer);
        // check if fully written?
        return paContinue;
    }

    // if we can't write data to ringbuffer, stop recording for now to early detect issues
    // TODO: determine what best to do here
    return paAbort;
}

PaError paEncodeInStream::ProtoOpenInputStream(PaDeviceIndex device)
{
    if (device == paDefaultDevice)
    {
        device = Pa_GetDefaultInputDevice();
    }

    inputParameters.device = device,
	inputParameters.channelCount = this->channels;
	inputParameters.sampleFormat = this->sampleFormat;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err;
	err = Pa_OpenStream(	&this->stream,
							&inputParameters,
							NULL,
							this->sampleRate,
							this->paCallbackFramesPerBuffer,
							paNoFlag,
							paStaticInputCallback,
							this
	);
	PaCHK("ProtoOpenInputStream", err);

	printf("ProtoOpenInputStream information:\n");
	log_pa_stream_info(this->stream, &inputParameters);
	return err;
}

int paEncodeInStream::GetRingBufferReadAvailable()
{
    return PaUtil_GetRingBufferReadAvailable(&this->rBufFromRT);
}

int paEncodeInStream::opusEncodeFloat(
    const float *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes)
{
    // TODO: The length of the encoded packet (in bytes) on success or a
    //          negative error code (see @ref opus_errorcodes) on failure.
    return opus_encode_float(this->encoder,
                        pcm,
                        frame_size,
                        data,
                        max_data_bytes);
}

int paEncodeInStream::OpusEncode(
    const opus_int16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes)
{
    // TODO: The length of the encoded packet (in bytes) on success or a
    //          negative error code (see @ref opus_errorcodes) on failure.
    return opus_encode(this->encoder,
                        pcm,
                        frame_size,
                        data,
                        max_data_bytes);
}

ring_buffer_size_t paEncodeInStream::ReadRingBuffer( PaUtilRingBuffer *rbuf, void *data, ring_buffer_size_t elementCount )
{
    return PaUtil_ReadRingBuffer(&this->rBufFromRT, data, elementCount);
}

PaError paEncodeInStream::StartStream()
{
    return Pa_StartStream(this->stream);
}

PaError paEncodeInStream::IsStreamActive()
{
    return Pa_IsStreamActive(this->stream);
}

PaError paEncodeInStream::StopStream()
{
    return Pa_StopStream(this->stream);
}

int paEncodeInStream::InitForDevice(PaDeviceIndex device)
{
    PaError pErr;
    int err;
    err = InitPaInputData();
    if (err != 0){
        printf("InitPaOutputData error\n");
        return -1;
    }
    
    pErr = ProtoOpenInputStream(device);
    PaCHK("ProtoOpenInputStream", pErr);

    return pErr;
}