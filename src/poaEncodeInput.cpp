#include "poaEncodeInput.h"

poaEncodeInput::poaEncodeInput(const char *name) : poaBase(name)
{
}

poaEncodeInput::~poaEncodeInput()
{
}

int poaEncodeInput::HandlePaStreamCallback(const void *inputBuffer,
                                           void *outputBuffer,
                                           unsigned long framesPerBuffer,
                                           const PaStreamCallbackTimeInfo *timeInfo,
                                           PaStreamCallbackFlags statusFlags)
{
    return paContinue;
}