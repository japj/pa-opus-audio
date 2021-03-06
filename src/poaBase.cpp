#include <stdarg.h>
#include "poaBase.h"

#ifdef __linux__
#include "pa_linux_alsa.h"
#endif

#ifdef __APPLE__
#include "pa_mac_core.h"
#endif

/* from #include "pa_util.h", which is not exported by default */
extern "C"
{
    void *PaUtil_AllocateMemory(long size);
    void PaUtil_FreeMemory(void *block);
}

poaBase::poaBase(const char *name) : name(name),
                                     stream(NULL),
                                     isCallbackRunning(false),
                                     opusSequenceNumber(0),
                                     rIntermediateCallbackBufData(NULL),
                                     rTransferDataBufData(NULL),
                                     transferDataElements(2) // TODO: determine/calculate good value
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        logPaError(err, "FAILED Pa_Initialize");
    }
    setupDefaultDeviceData(&inputData);
    setupDefaultDeviceData(&outputData);
}

poaBase::~poaBase()
{
    if (this->stream)
    {
        CloseStream();
    }
    PaError err = Pa_Terminate();
    if (err != paNoError)
    {
        logPaError(err, "FAILED Pa_Terminate");
    }
}

void poaBase::setupDefaultDeviceData(poaDeviceData *data)
{
    data->streamParams.device = paNoDevice;
    data->streamParams.channelCount = 1;
    data->streamParams.sampleFormat = paInt16;
    data->streamParams.suggestedLatency = 0;
    data->streamParams.hostApiSpecificStreamInfo = NULL;
    data->sampleRate = 48000;
    data->streamFlags = paNoFlag;
    data->callbackMaxFrameSize = 64;
    data->opusMaxFrameSize = 120; // 2.5ms@48kHz number of samples per channel in the input signal

    // ensure calculated fields are updated
    recalculateDeviceData(data);
}

void poaBase::recalculateDeviceData(poaDeviceData *data)
{
    data->sampleSize = Pa_GetSampleSize(data->streamParams.sampleFormat);
}

void *poaBase::AllocateMemory(long size)
{
    return PaUtil_AllocateMemory(size);
}
void poaBase::FreeMemory(void *block)
{
    PaUtil_FreeMemory(block);
}

int poaBase::calcSizeUpPow2(unsigned int v)
{ // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void poaBase::log(const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    printf("[%s]: %s", name.c_str(), buffer);
}

void poaBase::logPaError(PaError err, const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    log("ERROR {%s} %s", Pa_GetErrorText(err), buffer);
}

void poaBase::logOpusError(int err, const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    log("ERROR {%s} %s", opus_strerror(err), buffer);
}

void poaBase::log_pa_stream_info(PaStreamParameters *params)
{
    const PaDeviceInfo *deviceInfo;
    deviceInfo = Pa_GetDeviceInfo(params->device);
    const PaStreamInfo *streamInfo;
    streamInfo = Pa_GetStreamInfo(stream);
    double inputLatencySamples = streamInfo->inputLatency * streamInfo->sampleRate;
    double outputLatencySamples = streamInfo->outputLatency * streamInfo->sampleRate;

    log("DeviceId:         %d (%s)\n", params->device, deviceInfo->name);
    log("ChannelCount:     %d\n", params->channelCount);
    log("SuggestedLatency: %f\n", params->suggestedLatency);
    log("InputLatency:     %20f (%5.f samples)\n", streamInfo->inputLatency, inputLatencySamples);
    log("OutputLatency:    %20f (%5.f samples)\n", streamInfo->outputLatency, outputLatencySamples);
    log("SampleRate:       %.f\n", streamInfo->sampleRate);

    // TODO: this logging can be removed if adjusting in HandleOpenDeviceStream works correctly
    // WARN if input/output latency(in samples) is more than transferDataElements * opus max frame
    if (inputLatencySamples > transferDataElements * inputData.opusMaxFrameSize)
    {
        log("   !!! INPUT LATENCY WARNING !!! inputLatencySamples (%4.f) > transferDataElements * opusMaxFrameSize (%4d) !!!\n",
            inputLatencySamples, transferDataElements * inputData.opusMaxFrameSize);
    }
    if (outputLatencySamples > transferDataElements * outputData.opusMaxFrameSize)
    {
        log("   !!! OUTPUT LATENCY WARNING !!! outputLatencySamples (%4.f) > transferDataElements * opusMaxFrameSize (%4d) !!!\n",
            outputLatencySamples, transferDataElements * outputData.opusMaxFrameSize);
    }
}

PaError poaBase::StartStream()
{
    return Pa_StartStream(this->stream);
}

PaError poaBase::IsStreamActive()
{
    return Pa_IsStreamActive(this->stream);
}

PaError poaBase::StopStream()
{
    return Pa_StopStream(this->stream);
}

PaError poaBase::CloseStream()
{
    PaError err = paNoError;
    err = Pa_CloseStream(this->stream);
    this->stream = NULL;
    return err;
}

const PaStreamInfo *poaBase::GetStreamInfo()
{
    return Pa_GetStreamInfo(this->stream);
}

double poaBase::GetStreamCpuLoad()
{
    return Pa_GetStreamCpuLoad(this->stream);
}

int poaBase::paStaticStreamCallback(const void *inputBuffer,
                                    void *outputBuffer,
                                    unsigned long framesPerBuffer,
                                    const PaStreamCallbackTimeInfo *timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData)
{
    poaBase *data = (poaBase *)userData;

    if (!data->isCallbackRunning)
    {
        // ensure we know when the callback is actually running
        data->isCallbackRunning = true;
        data->log("paStaticStreamCallback:isCallbackRunning: %d\n", data->isCallbackRunning);
    }

    /* call the actual handler at c++ object side */
    return data->_HandlePaStreamCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

void poaBase::paStaticStreamFinishedCallback(void *userData)
{
    poaBase *data = (poaBase *)userData;

    if (data->isCallbackRunning)
    {
        // ensure we know when the callback is actually stopped
        data->isCallbackRunning = false;
        data->log("paStaticStreamFinishedCallback:isCallbackRunning: %d\n", data->isCallbackRunning);
    }

    data->_HandlePaStreamFinished();
}

bool poaBase::IsCallbackRunning()
{
    return this->isCallbackRunning;
}
void poaBase::_HandlePaStreamFinished()
{
    // empty implementation for derived class to actually customize
}

PaError poaBase::OpenInputDeviceStream(PaDeviceIndex inputDevice)
{
    return OpenDeviceStream(inputDevice);
}

PaError poaBase::OpenOutputDeviceStream(PaDeviceIndex outputDevice)
{
    return OpenDeviceStream(paNoDevice, outputDevice);
}

PaError poaBase::OpenDeviceStream(PaDeviceIndex inputDevice, PaDeviceIndex outputDevice)
{
    // TODO: make error handling robust, in case something fails inbetween start and finish, don't leave half an environment
    PaError err = paNoError;
    log("inputDevice(%d), outputDevice(%d)\n", inputDevice, outputDevice);

    PaDeviceIndex requestedInputDevice = (inputDevice == poaDefaultDevice) ? Pa_GetDefaultInputDevice() : inputDevice;
    PaDeviceIndex requestedOutputDevice = (outputDevice == poaDefaultDevice) ? Pa_GetDefaultOutputDevice() : outputDevice;

    log("requestedInputDevice(%d), requestedOutputDevice(%d)\n", requestedInputDevice, requestedOutputDevice);
    if (Pa_GetDeviceCount() == 0)
    {
        // no devices available at all
        return paInvalidDevice;
    }
    if (requestedInputDevice == paNoDevice && requestedOutputDevice == paNoDevice)
    {
        // no input and no output device was selected
        return paInvalidDevice;
    }
    if (requestedInputDevice >= Pa_GetDeviceCount())
    {
        // invalid input device
        return paInvalidDevice;
    }
    if (requestedOutputDevice >= Pa_GetDeviceCount())
    {
        // invalid output device
        return paInvalidDevice;
    }
    if (requestedOutputDevice == requestedInputDevice)
    {
        // duplex input/output with same divice not support yet
        return paInvalidDevice;
    }

    PaStreamParameters *inputParams = NULL;
    PaStreamParameters *outputParams = NULL;

    double sampleRate;
    unsigned long callbackMaxFrameSize;
    PaStreamFlags streamFlags;
    int sampleSize;

#ifdef __APPLE__
    /** paMacCoreMinimizeCPU is a setting to minimize CPU usage, even if that means interrupting the device (by changing device parameters) */
    PaMacCoreStreamInfo macInfo;
    PaMacCore_SetupStreamInfo(&macInfo, paMacCoreMinimizeCPU);
#endif

    if (requestedInputDevice != paNoDevice)
    {
        // setup requested input device parameters
        inputData.streamParams.device = requestedInputDevice,
        inputData.streamParams.suggestedLatency = Pa_GetDeviceInfo(inputData.streamParams.device)->defaultLowInputLatency;
#ifdef __APPLE__
        inputData.streamParams.hostApiSpecificStreamInfo = &macInfo;
#endif
        inputParams = &inputData.streamParams;

        sampleRate = inputData.sampleRate;
        callbackMaxFrameSize = inputData.callbackMaxFrameSize;
        streamFlags = inputData.streamFlags;
        sampleSize = inputData.sampleSize;
    }
    if (requestedOutputDevice != paNoDevice)
    {
        // setup requested output device parameters
        outputData.streamParams.device = requestedOutputDevice;
        outputData.streamParams.suggestedLatency = Pa_GetDeviceInfo(outputData.streamParams.device)->defaultLowOutputLatency;
#ifdef __APPLE__
        outputData.streamParams.hostApiSpecificStreamInfo = &macInfo;
#endif
        outputParams = &outputData.streamParams;

        sampleRate = outputData.sampleRate;
        callbackMaxFrameSize = outputData.callbackMaxFrameSize;
        streamFlags = outputData.streamFlags;
        sampleSize = outputData.sampleSize;
    }
    /*
    log("Pa_OpenStream:\n");
    log("Pa_OpenStream: inputParams             %x\n", inputParams);
    log("Pa_OpenStream: outputParams            %x\n", outputParams);
    log("Pa_OpenStream: sampleRate              %5.f\n", sampleRate);
    log("Pa_OpenStream: callbackMaxFrameSize    %d\n", callbackMaxFrameSize);
    log("Pa_OpenStream: streamFlags             %x\n", streamFlags);
    */
    err = Pa_OpenStream(&this->stream,
                        inputParams,
                        outputParams,
                        sampleRate,
                        callbackMaxFrameSize,
                        streamFlags,
                        this->paStaticStreamCallback, this);
    PaLOGERR(err, "Pa_OpenStream\n");
    if (inputParams)
    {
        err = HandleOpenDeviceStream();
        PaLOGERR(err, "HandleOpenDeviceStream input\n");

        log_pa_stream_info(inputParams);

        // this needs tuning or can it be calculated?
        // TODO: remove channelCount after encoding data, this is just needed to have enough space for stereo sound?

        //intermediateRingBufferFrames = calcSizeUpPow2(2 * inputData.opusMaxFrameSize);
        intermediateRingBufferFrames = calcSizeUpPow2(1 * inputData.opusMaxFrameSize + inputData.callbackMaxFrameSize);
        intermediateRingBufferSize = intermediateRingBufferFrames * inputData.sampleSize * inputData.streamParams.channelCount;
    }
    if (outputParams)
    {
        err = HandleOpenDeviceStream();
        PaLOGERR(err, "HandleOpenDeviceStream output\n");

        log_pa_stream_info(outputParams);

        // NOTE: any "crackling sound" can be recognized in the
        // logging by [output]: writeEncodedOpusFrame:FAILED to write full opusFrame, written only (112) frames

        //intermediateRingBufferFrames = calcSizeUpPow2(2 * outputData.opusMaxFrameSize);
        intermediateRingBufferFrames = calcSizeUpPow2(1 * outputData.opusMaxFrameSize + outputData.callbackMaxFrameSize);
        intermediateRingBufferSize = intermediateRingBufferFrames * outputData.sampleSize * outputData.streamParams.channelCount;
    }

    err = Pa_SetStreamFinishedCallback(this->stream, this->paStaticStreamFinishedCallback);
    PaLOGERR(err, "Pa_SetStreamFinishedCallback\n");

    log("OpenDeviceStream: intermediateRingBufferFrames(%d), intermediateRingBufferSize(%d) sampleSize(%d)\n", intermediateRingBufferFrames, intermediateRingBufferSize, sampleSize);

    /*** INTERMEDIATE RINGBUFFER ALLOCATION */
    rIntermediateCallbackBufData = AllocateMemory(intermediateRingBufferSize);
    if (rIntermediateCallbackBufData == NULL)
    {
        log("rIntermediateCallbackBufData AllocateMemory FAILED\n");
        return paInsufficientMemory;
    }
    err = PaUtil_InitializeRingBuffer(&rIntermediateCallbackBuf, sampleSize, intermediateRingBufferFrames, rIntermediateCallbackBufData);
    if (err != 0)
    {
        log("OpenDeviceStream: PaUtil_InitializeRingBuffer rIntermediateCallbackBuf failed with %d\n", err);
    }

    /** TRANSFERDATA RINBUFFER ALLOCATION */
    transferDataRingBufferSize = sizeof(poaCallbackTransferData) * transferDataElements;
    rTransferDataBufData = AllocateMemory(transferDataRingBufferSize);
    if (rTransferDataBufData == NULL)
    {
        log("rTransferDataBufData AllocateMemory FAILED\n");
        return paInsufficientMemory;
    }
    err = PaUtil_InitializeRingBuffer(&rTransferDataBuf, sizeof(poaCallbackTransferData), transferDataElements, rTransferDataBufData);
    if (err != 0)
    {
        log("OpenDeviceStream: PaUtil_InitializeRingBuffer rTransferDataBuf failed with %d\n", err);
    }

#ifdef __linux__
    /** Instruct Alsa to enable real-time priority when starting the audio thread.
     *
     * the audio callback thread will be created with the FIFO scheduling policy, which is suitable for realtime operation.
     * 
     * NOTE: this currently assumes we are actually only using alsa on linux (which is true since our copy of portaudio has that only enabled on linux)
     **/
    PaAlsa_EnableRealtimeScheduling(this->stream, 1);
#endif

    return err;
}

void poaBase::setName(const char *name)
{
    this->name = name;
}