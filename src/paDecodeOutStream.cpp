#include <memory.h>
#include <stdio.h>
#include "paDecodeOutStream.h"

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int paStaticOutputCallback(const void *inputBuffer,
                                  void *outputBuffer,
                                  unsigned long framesPerBuffer,
                                  const PaStreamCallbackTimeInfo *timeInfo,
                                  PaStreamCallbackFlags statusFlags,
                                  void *userData)
{
    paDecodeOutStream *data = (paDecodeOutStream *)userData;

    /* call the paDecodeOutStream object counterpart */
    return data->paOutputCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

paDecodeOutStream::paDecodeOutStream(/* args */)
{
    sampleRate = 48000;              // opus only allows certain sample rates for encoding
    sampleFormat = paInt16;          //paFloat32 or paInt16;
    channels = 1;                    //?2; // needs to be 1 if you want to output recorded audio directly without opus enc/dec in order to get same amount of data/structure mono/stereo
                                     // 2 gives garbled sound?
    paCallbackFramesPerBuffer = 128; /* since opus decodes 120 frames, this is closests to how our latency is going to be
                                                // frames per buffer for OS Audio buffer*/
    opusMaxFrameSize = 5760;         // maximum packet duration (120ms; 5760 for 48kHz)

    maxRingBufferSamples = 0;
    stream = NULL;
    decoder = NULL;
    firstFramesize = 0;
    firstDecodeCalled = false;

    setupPa();
}

paDecodeOutStream::~paDecodeOutStream()
{
    // TODO: cleanup all memory
    printf("paDecodeOutStream::~paDecodeOutStream called\n");
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

int paDecodeOutStream::InitPaOutputData()
{
    int err;

    if (this->sampleRate == 48000)
    {
        this->decoder = opus_decoder_create(this->sampleRate, this->channels, &err);
        if (this->decoder == NULL)
        {
            fprintf(stderr, "opus_decoder_create: %s\n",
                    opus_strerror(err));
            return -1;
        }
    }

    return 0;
}

int paDecodeOutStream::paOutputCallback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags)
{
    (void)inputBuffer; /* Prevent "unused variable" warnings. */

    /* Reset output data first */
    memset(outputBuffer, 0, framesPerBuffer * this->channels * this->sampleSizeSizeBytes);

    unsigned long availableInReadBuffer = (unsigned long)PaUtil_GetRingBufferReadAvailable(&this->rBufToRT);
    ring_buffer_size_t actualFramesRead = 0;

    // always playback available audio, even if it is not enough to fill entire framesPerBuffer requested
    unsigned long toCopyData = (framesPerBuffer > availableInReadBuffer) ? availableInReadBuffer : framesPerBuffer;
    if (toCopyData > 0)
    {
        actualFramesRead = PaUtil_ReadRingBuffer(&this->rBufToRT, outputBuffer, toCopyData);

        // if actualFramesRead < framesPerBuffer then we read not enough data
    }
    else
    {
        if (firstDecodeCalled)
        {
            printf("paOutputCallback: not enough data available needed(%ld), available(%ld)\n", framesPerBuffer, availableInReadBuffer);
        }
    }
    if (toCopyData > 0 && actualFramesRead != framesPerBuffer)
    {
        if (firstDecodeCalled)
        {
            printf("paOutputCallback: partial read(%d), needed(%ld)\n", actualFramesRead, framesPerBuffer);
        }
    }

    // TODO: if framesPerBuffer > availableInReadBuffer we have a buffer underrun, notify?
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
    err = Pa_OpenStream(&this->stream,
                        NULL,
                        &outputParameters,
                        this->sampleRate,
                        this->paCallbackFramesPerBuffer,
                        paNoFlag,
                        paStaticOutputCallback,
                        this);
    PaCHK("ProtoOpenOutputStream", err);

    printf("ProtoOpenOutputStream information:\n");
    log_pa_stream_info(this->stream, &outputParameters);

    const PaStreamInfo *streamInfo;
    streamInfo = Pa_GetStreamInfo(stream);
    int outputLatency = streamInfo->outputLatency * streamInfo->sampleRate * 2;

    maxRingBufferSamples = calcSizeUpPow2(outputLatency); // needed for PaUtil RingBuffer to work
    printf("maxRingBufferSamples: %ld\n", maxRingBufferSamples);

    /*int callbackBasedLatency = 

    maxRingBufferSamples = (opaCallbackFramesPerBuffer > opusMaxFrameSize + paCallbackFramesPerBuffer) ? 
                                outputLatency + paCallbackFramesPerBuffer : 
                                opusMaxFrameSize + paCallbackFramesPerBuffer;
    */

    // TODO: calculate optimal ringbuffer size
    // note opusDecodeBuffer is opusMaxFrameSize * sizeof sampleFormatInBytes

    this->sampleSizeSizeBytes = Pa_GetSampleSize(this->sampleFormat);
    long bufferSize = sampleSizeSizeBytes * this->maxRingBufferSamples;
    this->rBufToRTData = ALLIGNEDMALLOC(bufferSize);
    err = PaUtil_InitializeRingBuffer(&this->rBufToRT, sampleSizeSizeBytes, this->maxRingBufferSamples, this->rBufToRTData);
    PaCHK("paDecodeOutStream:PaUtil_InitializeRingBuffer", err);

    this->opusDecodeBuffer = ALLIGNEDMALLOC(sampleSizeSizeBytes * this->opusMaxFrameSize);

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
    int decode_fec)
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
    int decode_fec)
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

ring_buffer_size_t paDecodeOutStream::WriteRingBuffer(const void *data, ring_buffer_size_t elementCount)
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

PaError paDecodeOutStream::CloseStream()
{
    PaError err = paNoError;
    if (this->stream)
    {
        err = Pa_CloseStream(this->stream);
        this->stream = NULL;
    }
    return err;
}

int paDecodeOutStream::InitForDevice(PaDeviceIndex device)
{
    PaError pErr;
    int err;
    err = InitPaOutputData();
    if (err != 0)
    {
        printf("InitPaOutputData error\n");
        return -1;
    }

    pErr = ProtoOpenOutputStream(device);
    PaCHK("ProtoOpenOutputStream", pErr);

    return pErr;
}

int paDecodeOutStream::DecodeDataIntoPlayback(void *data, opus_int32 len, int dec)
{
    // guard against possible multiple DecodeWorkers interacting
    std::lock_guard<std::mutex> guard(decodeDataIntoPlaybackMutex);
    if (!firstDecodeCalled)
    {
        firstDecodeCalled = true;
    }

    int ringBufferWriteAvailable = this->GetRingBufferWriteAvailable();

    void *writeBufferPtr;
    int toWriteFrameCount;
    int decodedFrameCount;
    ring_buffer_size_t framesWritten = 0;

    if (sampleRate == 48000)
    {

        writeBufferPtr = this->opusDecodeBuffer;

        if (sampleFormat == paFloat32)
        {
            decodedFrameCount = this->opusDecodeFloat((unsigned char *)data,
                                                      len,
                                                      (float *)this->opusDecodeBuffer,
                                                      this->opusMaxFrameSize,
                                                      dec);
        }
        else
        {
            // we assume paInt16;
            decodedFrameCount = this->OpusDecode((unsigned char *)data,
                                                 len,
                                                 (opus_int16 *)this->opusDecodeBuffer,
                                                 this->opusMaxFrameSize,
                                                 dec);
        }

        if (firstFramesize == 0)
        {
            firstFramesize = decodedFrameCount;
            printf("\nDecodeDataIntoPlayback::firstFramesize(%d)\n", firstFramesize);
        }
        if (decodedFrameCount < 0)
        {
            printf("DecodeDataIntoPlayback:OpusDecode failed\n");
        }
    }
    else
    {
        // only support encode/decoding with opus
        // so for now treat this as "pass-through" audio
        writeBufferPtr = data;
        decodedFrameCount = len;
    }

    toWriteFrameCount = decodedFrameCount > ringBufferWriteAvailable ? ringBufferWriteAvailable : decodedFrameCount;

    // we need to decode all data in sequence order
    // however, if there is not enough place in the Pa RingBuffer then for now we will just drop any data so audio playback loss might occur
    if (toWriteFrameCount > 0)
    {
        framesWritten = this->WriteRingBuffer(writeBufferPtr, toWriteFrameCount);
    }
    if (framesWritten != toWriteFrameCount)
    {
        printf("DecodeDataIntoPlayback: writeRingBuffer tried(%d) actual(%d)\n", toWriteFrameCount, framesWritten);
    }
    if (framesWritten != decodedFrameCount)
    {
        printf("DecodeDataIntoPlayback: decoded(%d) written(%d)\n", decodedFrameCount, framesWritten);
    }
    // framesWritten could be less than GetRingBufferWriteAvailable -> TODO notify?

    return framesWritten;
}