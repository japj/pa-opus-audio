#include <string.h>
#include "poaDecodeOutput.h"

poaDecodeOutput::poaDecodeOutput(const char *name) : poaBase(name)
{
}

poaDecodeOutput::~poaDecodeOutput()
{
}

int poaDecodeOutput::HandlePaStreamCallback(const void *inputBuffer,
                                            void *outputBuffer,
                                            unsigned long framesPerBuffer,
                                            const PaStreamCallbackTimeInfo *timeInfo,
                                            PaStreamCallbackFlags statusFlags)
{
    /* Reset output data first */
    memset(outputBuffer, 0, framesPerBuffer * this->outputData.streamParams.channelCount * this->outputData.sampleSize);
    return paContinue;
}