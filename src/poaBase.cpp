#include "poaBase.h"

poaBase::poaBase(const char *name) : name(name), stream(NULL), isCallbackRunning(false)
{
    Pa_Initialize();
    setupDefaultDeviceData(&inputData);
    setupDefaultDeviceData(&outputData);
}

poaBase::~poaBase()
{
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
    data->framesPerBuffer = 64;

    // ensure calculated fields are updated
    recalculateDeviceData(data);
}

void poaBase::recalculateDeviceData(poaDeviceData *data)
{
    data->sampleSize = Pa_GetSampleSize(data->streamParams.sampleFormat);
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
    return data->HandlePaStreamCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

void poaBase::paStaticStreamFinishedCallback(void *userData)
{
    poaBase *data = (poaBase *)userData;

    if (data->isCallbackRunning)
    {
        // ensure we know when the callback is actually stopped
        data->isCallbackRunning = false;
    }

    data->HandlePaStreamFinished();
}

bool poaBase::IsCallbackRunning()
{
    return this->isCallbackRunning;
}
void poaBase::HandlePaStreamFinished()
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

    PaStreamParameters *inputParams = NULL;
    PaStreamParameters *outputParams = NULL;

    if (requestedInputDevice != paNoDevice)
    {
        // setup requested input device parameters
        inputData.streamParams.device = requestedInputDevice,
        inputData.streamParams.suggestedLatency = Pa_GetDeviceInfo(inputData.streamParams.device)->defaultLowInputLatency;
        inputParams = &inputData.streamParams;
    }
    if (requestedOutputDevice != paNoDevice)
    {
        // setup requested output device parameters
        outputData.streamParams.device = requestedOutputDevice;
        outputData.streamParams.suggestedLatency = Pa_GetDeviceInfo(outputData.streamParams.device)->defaultLowOutputLatency;
        outputParams = &outputData.streamParams;
    }

    err = Pa_OpenStream(&this->stream,
                        inputParams,
                        outputParams,
                        this->inputData.sampleRate,
                        this->inputData.framesPerBuffer,
                        this->inputData.streamFlags,
                        this->paStaticStreamCallback, this);
    PaLOGERR(err, "Pa_OpenStream\n");

    err = Pa_SetStreamFinishedCallback(this->stream, this->paStaticStreamFinishedCallback);
    PaLOGERR(err, "Pa_SetStreamFinishedCallback\n");

    return err;
}