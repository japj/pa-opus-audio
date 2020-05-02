#include <stdio.h>
#include <memory.h>
#include "paEncodeInStream.h"

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paStaticInputCallback(const void *inputBuffer,
                                 void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    paEncodeInStream *data = (paEncodeInStream *)userData;

    /* call the paEncodeInStream object counterpart */

    return data->paInputCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

paEncodeInStream::paEncodeInStream(/* args */)
{
    sampleRate = 48000;     // opus only allows certain sample rates for encoding
    sampleFormat = paInt16; //paFloat32 or paInt16;
    channels = 1;           //?2; // needs to be 1 if you want to output recorded audio directly without opus enc/dec in order to get same amount of data/structure mono/stereo
                            // 2 gives garbled sound?

    paCallbackFramesPerBuffer = 64; /* since opus decodes 120 frames, this is closests to how our latency is going to be
                                                // frames per buffer for OS Audio buffer*/
    opusMaxFrameSize = 120;         // 2.5ms@48kHz number of samples per channel in the input signal

    //opusMaxFrameSize                = 2880;     // 2280 is sample buffer size for decoding at at 48kHz with 60ms
    // for better analysis of the audio I am sending with 60ms from opusrtp

    // TODO: latency is more important than correctness of audio stream
    // if for whatever reason the ringBuffer gets full, we do not want to fill it with audio that will lag behind
    // thus we limit the maxRingBufferSamples to 10ms for now (4 * opus frame)
    maxRingBufferSamples = calcSizeUpPow2(opusMaxFrameSize * 4); // TODO: calculate optimal ringbuffer size
                                                                 // note opusDecodeBuffer is opusMaxFrameSize * sizeof sampleFormatInBytes
    userDataCallbackOpusFrameAvailable = NULL;
    userCallbackOpusFrameAvailable = NULL;
    framesWrittenSinceLastCallback = 0;

    setupPa();
}

paEncodeInStream::~paEncodeInStream()
{
    // TODO: cleanup all memory
    printf("paEncodeInStream::~paEncodeInStream called\n");
    if (this->stream)
    {
        if (Pa_IsStreamActive(this->stream) == 1)
        {
            this->StopStream();
        }
        this->CloseStream();
    }

    terminatePa();
}

int paEncodeInStream::InitPaInputData()
{
    int err;

    this->sampleSizeSizeBytes = Pa_GetSampleSize(this->sampleFormat);
    long bufferSize = sampleSizeSizeBytes * this->maxRingBufferSamples;
    this->rBufFromRTData = ALLIGNEDMALLOC(bufferSize);
    err = PaUtil_InitializeRingBuffer(&this->rBufFromRT, sampleSizeSizeBytes, this->maxRingBufferSamples, this->rBufFromRTData);
    PaCHK("paEncodeInStream:PaUtil_InitializeRingBuffer", err);

    if (this->sampleRate == 48000)
    {
        this->encoder = opus_encoder_create(this->sampleRate, this->channels, OPUS_APPLICATION_AUDIO,
                                            &err);
        if (this->encoder == NULL)
        {
            fprintf(stderr, "opus_encoder_create: %s\n",
                    opus_strerror(err));
            return -1;
        }
    }

    this->opusEncodeBuffer = ALLIGNEDMALLOC(bufferSize);

    return 0;
}

int paEncodeInStream::paInputCallback(const void *inputBuffer,
                                      void *outputBuffer,
                                      unsigned long framesPerBuffer,
                                      const PaStreamCallbackTimeInfo *timeInfo,
                                      PaStreamCallbackFlags statusFlags)
{
    (void)outputBuffer; /* Prevent "unused variable" warnings. */

    unsigned long availableWriteFramesInRingBuffer = (unsigned long)PaUtil_GetRingBufferWriteAvailable(&this->rBufFromRT);
    ring_buffer_size_t written = 0;

    // for now, we only write to the ring buffer if enough space is available
    if (framesPerBuffer <= availableWriteFramesInRingBuffer)
    {
        written = PaUtil_WriteRingBuffer(&this->rBufFromRT, inputBuffer, framesPerBuffer);
        // check if fully written?

        framesWrittenSinceLastCallback += written;
        // check if a full opus frame is available and notify if that is the case
        // while loop here in case framesPerBuffer > opusMaxFrameSize, which means we should trigger multiple times
        while (framesWrittenSinceLastCallback >= opusMaxFrameSize)
        {
            if (userCallbackOpusFrameAvailable)
            {
                // notify user that one opus frame is available
                userCallbackOpusFrameAvailable(userDataCallbackOpusFrameAvailable);
            }

            framesWrittenSinceLastCallback -= opusMaxFrameSize;
        }
        return paContinue;
    }

    // if we can't write data to ringbuffer, stop recording for now to early detect issues
    // TODO: determine what best to do here
    // hook callback into Pa_SetStreamFinishedCallback so we get triggers if the stream is ended

    // ensure for now that audio recording will continue forever
    // e.g. during testing selecting output on console could block consuming callbacks
    return paContinue; // paAbort;
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
    err = Pa_OpenStream(&this->stream,
                        &inputParameters,
                        NULL,
                        this->sampleRate,
                        this->paCallbackFramesPerBuffer,
                        paNoFlag,
                        paStaticInputCallback,
                        this);
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

ring_buffer_size_t paEncodeInStream::ReadRingBuffer(PaUtilRingBuffer *rbuf, void *data, ring_buffer_size_t elementCount)
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

PaError paEncodeInStream::CloseStream()
{
    PaError err = paNoError;
    if (this->stream)
    {
        err = Pa_CloseStream(this->stream);
        this->stream = NULL;
    }
    return err;
}

int paEncodeInStream::InitForDevice(PaDeviceIndex device)
{
    PaError pErr;
    int err;
    err = InitPaInputData();
    if (err != 0)
    {
        printf("InitPaOutputData error\n");
        return -1;
    }

    pErr = ProtoOpenInputStream(device);
    PaCHK("ProtoOpenInputStream", pErr);

    return pErr;
}

int paEncodeInStream::EncodeRecordingIntoData(void *data, opus_int32 len)
{
    // check minimum available space needed?

    int encodedPacketSize;
    ring_buffer_size_t framesRead;

    // TODO: design API

    ring_buffer_size_t availableInInputBuffer = this->GetRingBufferReadAvailable();
    if (availableInInputBuffer < opusMaxFrameSize)
    {
        // don' try to read/encode if no full opus frame is available;
        return 0;
    }

    // can only run opus encoding/decoding on 48000 samplerate
    if (sampleRate == 48000)
    {

        framesRead = this->ReadRingBuffer(&this->rBufFromRT, this->opusEncodeBuffer, opusMaxFrameSize);

        // use float32 or int16 opus encoder/decoder
        if (sampleFormat == paFloat32)
        {
            // encode audio
            encodedPacketSize = this->opusEncodeFloat(
                (float *)this->opusEncodeBuffer,
                opusMaxFrameSize,
                (unsigned char *)data,
                len);
        }
        else
        {
            // encode audio
            encodedPacketSize = this->OpusEncode(
                (opus_int16 *)this->opusEncodeBuffer,
                opusMaxFrameSize,
                (unsigned char *)data,
                len);
        }
    }
    else
    {
        // only support encoding/decoding with opus
        // so for now only treat this as "pass-through" audio
        encodedPacketSize = opusMaxFrameSize * this->sampleSizeSizeBytes;
        if (encodedPacketSize <= len)
        {
            framesRead = this->ReadRingBuffer(&this->rBufFromRT, data, opusMaxFrameSize);

            // TODO framesRead should be equal to opusMaxFrameSize
        }
        else
        {
            // if buffer does not fit, give error code back
            encodedPacketSize = -1;
        }
    }

    return encodedPacketSize;
}

int paEncodeInStream::GetUncompressedBufferSizeBytes()
{
    return this->sampleSizeSizeBytes * this->opusMaxFrameSize;
}

void paEncodeInStream::setUserCallbackOpusFrameAvailable(paEncodeInStreamOpusFrameAvailableCallback cb, void *userData)
{
    this->userCallbackOpusFrameAvailable = cb;
    this->userDataCallbackOpusFrameAvailable = userData;
}
