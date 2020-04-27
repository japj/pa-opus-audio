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
    sampleRate                      = 48000;    // opus only allows certain sample rates for encoding
    sampleFormat                    = paInt16;  //paFloat32 or paInt16;
    channels                        = 1;        //?2; // needs to be 1 if you want to output recorded audio directly without opus enc/dec in order to get same amount of data/structure mono/stereo
                                                // 2 gives garbled sound?
    bufferElements                  = 4096;     // TODO: calculate optimal ringbuffer size
                                                // note opusDecodeBuffer is opusMaxFrameSize * sizeof sampleFormatInBytes
    paCallbackFramesPerBuffer       = 64;       /* since opus decodes 120 frames, this is closests to how our latency is going to be
                                                // frames per buffer for OS Audio buffer*/
    opusMaxFrameSize                = 120;      // 2.5ms@48kHz number of samples per channel in the input signal
}

paDecodeOutStream::~paDecodeOutStream()
{
    // TODO: cleanup all memory
}

int paDecodeOutStream::InitPaOutputData()
{
    int err;

    long writeDataBufElementCount = this->bufferElements;
    long writeSampleSize = Pa_GetSampleSize(sampleFormat);
    long bufferSize = writeSampleSize * writeDataBufElementCount;
    this->rBufToRTData = ALLIGNEDMALLOC(bufferSize);
    PaUtil_InitializeRingBuffer(&this->rBufToRT, writeSampleSize, writeDataBufElementCount, this->rBufToRTData);
    this->frameSizeBytes = writeSampleSize;

    if (this->sampleRate == 48000) {
        this->decoder = opus_decoder_create(this->sampleRate, this->channels, &err);
        if (this->decoder == NULL) {
            fprintf(stderr, "opus_decoder_create: %s\n",
                opus_strerror(err));
            return -1;
        }
    }

    this->opusDecodeBuffer = ALLIGNEDMALLOC(bufferSize);

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

PaError paDecodeOutStream::ProtoOpenOutputStream(PaDeviceIndex device)
{
    if (device == paDefaultDevice)
    {
        device = Pa_GetDefaultOutputDevice();
    }
	
    outputParameters.device = device;
	outputParameters.channelCount = this->channels;
	outputParameters.sampleFormat = this->sampleFormat;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaError err;
	err = Pa_OpenStream(	&this->stream,
							NULL,
							&outputParameters,
							this->sampleRate,
							this->paCallbackFramesPerBuffer,
							paNoFlag,
							paStaticOutputCallback,
							this
	);
	PaCHK("ProtoOpenOutputStream", err);

	printf("ProtoOpenOutputStream information:\n");
	log_pa_stream_info(this->stream, &outputParameters);
	return err;
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
    // TODO: Number of decoded samples or Error codes
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
    // TODO: Number of decoded samples or Error codes
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

int paDecodeOutStream::InitForDevice(PaDeviceIndex device)
{
    PaError pErr;
    int err;
    err = InitPaOutputData();
    if (err != 0){
        printf("InitPaOutputData error\n");
        return -1;
    }
    
    pErr = ProtoOpenOutputStream(device);
    PaCHK("ProtoOpenOutputStream", pErr);

    return 0;
}

int paDecodeOutStream::DecodeDataIntoPlayback(void *data, opus_int32 len, int dec)
{
    void *writeBufferPtr;
    int toWriteFrameCount;
    ring_buffer_size_t framesWritten;

    if (sampleRate == 48000) {
        writeBufferPtr = this->opusDecodeBuffer;

        if (sampleFormat == paFloat32) {
            toWriteFrameCount = this->opusDecodeFloat(  (unsigned char*)data,
                                                        len,
                                                        (float *)this->opusDecodeBuffer,
                                                        this->opusMaxFrameSize,
                                                        dec);
        }
        else
        {
            // we assume paInt16;
            toWriteFrameCount = this->OpusDecode(   (unsigned char*)data,
                                                    len,
                                                    (opus_int16*)this->opusDecodeBuffer,
                                                    this->opusMaxFrameSize,
                                                    dec);
        }
        
    }
    else {
        // only support encode/decoding with opus
        // so for now treat this as "pass-through" audio
        writeBufferPtr = data;
        toWriteFrameCount = len;
    }

    // we need to decode all data in sequence order
    // however, if there is not enough place in the Pa RingBuffer then for now we will just drop any data so audio playback loss might occur
    framesWritten = this->WriteRingBuffer(writeBufferPtr, toWriteFrameCount);
    // framesWritten could be less than GetRingBufferWriteAvailable -> TODO notify?

    return framesWritten;
}