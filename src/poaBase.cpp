#include <stdarg.h>
#include "poaBase.h"

/* from #include "pa_util.h", which is not exported by default */
extern "C"
{
    void *PaUtil_AllocateMemory(long size);
    void PaUtil_FreeMemory(void *block);
}

poaBase::poaBase(const char *name) : name(name), stream(NULL), isCallbackRunning(false), opusSequenceNumber(0)
{
    Pa_Initialize();
    setupDefaultDeviceData(&inputData);
    setupDefaultDeviceData(&outputData);
}

poaBase::~poaBase()
{
    if (this->stream)
    {
        CloseStream();
    }
    Pa_Terminate();
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
    log("{%s} %s", Pa_GetErrorText(err), buffer);
}

void poaBase::log_pa_stream_info(PaStreamParameters *params)
{
    const PaDeviceInfo *deviceInfo;
    deviceInfo = Pa_GetDeviceInfo(params->device);
    const PaStreamInfo *streamInfo;
    streamInfo = Pa_GetStreamInfo(stream);

    log("DeviceId:         %d (%s)\n", params->device, deviceInfo->name);
    log("ChannelCount:     %d\n", params->channelCount);
    log("SuggestedLatency: %f\n", params->suggestedLatency);
    log("InputLatency:     %20f (%5.f samples)\n", streamInfo->inputLatency, streamInfo->inputLatency * streamInfo->sampleRate);
    log("OutputLatency:    %20f (%5.f samples)\n", streamInfo->outputLatency, streamInfo->outputLatency * streamInfo->sampleRate);
    log("SampleRate:       %.f\n", streamInfo->sampleRate);
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

    if (requestedInputDevice != paNoDevice)
    {
        // setup requested input device parameters
        inputData.streamParams.device = requestedInputDevice,
        inputData.streamParams.suggestedLatency = Pa_GetDeviceInfo(inputData.streamParams.device)->defaultLowInputLatency;
        inputParams = &inputData.streamParams;

        sampleRate = inputData.sampleRate;
        callbackMaxFrameSize = inputData.callbackMaxFrameSize;
        streamFlags = inputData.streamFlags;
    }
    if (requestedOutputDevice != paNoDevice)
    {
        // setup requested output device parameters
        outputData.streamParams.device = requestedOutputDevice;
        outputData.streamParams.suggestedLatency = Pa_GetDeviceInfo(outputData.streamParams.device)->defaultLowOutputLatency;
        outputParams = &outputData.streamParams;

        sampleRate = outputData.sampleRate;
        callbackMaxFrameSize = outputData.callbackMaxFrameSize;
        streamFlags = outputData.streamFlags;
    }

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
        log_pa_stream_info(inputParams);
    }
    if (outputParams)
    {
        log_pa_stream_info(outputParams);
    }

    err = Pa_SetStreamFinishedCallback(this->stream, this->paStaticStreamFinishedCallback);
    PaLOGERR(err, "Pa_SetStreamFinishedCallback\n");

    err = HandleOpenDeviceStream();
    PaLOGERR(err, "HandleOpenDeviceStream\n");
    return err;
}