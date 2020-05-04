#include <string.h>
#include "poaDecodeOutput.h"

poaDecodeOutput::poaDecodeOutput(const char *name) : poaBase(name)
{
    //is this needed? outputData.streamParams.channelCount = 2;
}

poaDecodeOutput::~poaDecodeOutput()
{
}

int poaDecodeOutput::_HandlePaStreamCallback(const void *inputBuffer,
                                             void *outputBuffer,
                                             unsigned long framesPerBuffer,
                                             const PaStreamCallbackTimeInfo *timeInfo,
                                             PaStreamCallbackFlags statusFlags)
{
    /* Reset output data first */
    memset(outputBuffer, 0, framesPerBuffer * this->outputData.streamParams.channelCount * this->outputData.sampleSize);

    // since we eventually going to decode data and transfer from compressed chunks,
    // we will only tranfer data if it fully fits in intermediate buffer space
    // since framesPerBuffer < opusFrameSize(120) we need to store the content of the buffer
    // each time there is enough space in intermediate and we have encoded data available
    // then we transfer the encoded data for consumption to tranferBuffer

    // 1. if we have enough space in intermediate buffer for a decoded opus_frame, start decoding
    //    - need to read a poaDeviceData structure

    // TODO check with poaEncodeInput for consistency
    // TODO: what to do with sequence_numbers that have skipped content?

    // x. put decoded opus frame into intermediate ringbuffer

    // 8. store all possible frames from intermediate ringbuffer into outputBuffer
    // TODO: could
    ring_buffer_size_t read_available;
    read_available = PaUtil_GetRingBufferReadAvailable(&rIntermediateCallbackBuf);
    unsigned long toReadFrames = framesPerBuffer > read_available ? read_available : framesPerBuffer;
    if (toReadFrames != framesPerBuffer)
    {
        log("_HandlePaStreamCallback: SKIPPING playback, no room for (%d) intermediate frames\n", framesPerBuffer - read_available);
    }
    return paContinue;
}

PaError poaDecodeOutput::HandleOpenDeviceStream()
{

    return paNoError;
}