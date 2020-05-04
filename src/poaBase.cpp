#include "poaBase.h"

poaBase::poaBase(const char *name) : name(name), stream(NULL), inputDevice(paNoDevice), outputDevice(paNoDevice)
{
    Pa_Initialize();
}

poaBase::~poaBase()
{
    Pa_Terminate();
}

void poaBase::log(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    printf("[%s]: %s", name.c_str(), buffer);
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

int poaBase::paStaticCallback(const void *inputBuffer,
                              void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData)
{
    poaBase *data = (poaBase *)userData;

    /* call the actual handler at c++ object side */
    return data->HandlePaCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
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
    log("inputDevice(%d), outputDevice(%d)\n", inputDevice, outputDevice);

    return paNoError;
}