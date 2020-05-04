#include "poaDecodeOutput.h"

poaDecodeOutput::poaDecodeOutput(const char *name) : poaBase(name)
{
}

poaDecodeOutput::~poaDecodeOutput()
{
}

int poaDecodeOutput::HandlePaCallback(const void *inputBuffer,
                                      void *outputBuffer,
                                      unsigned long framesPerBuffer,
                                      const PaStreamCallbackTimeInfo *timeInfo,
                                      PaStreamCallbackFlags statusFlags)
{
    return paContinue;
}