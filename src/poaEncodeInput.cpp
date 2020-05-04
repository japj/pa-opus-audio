#include "poaEncodeInput.h"

poaEncodeInput::poaEncodeInput(const char *name) : poaBase(name)
{
}

poaEncodeInput::~poaEncodeInput()
{
}

int poaEncodeInput::_HandlePaStreamCallback(const void *inputBuffer,
                                            void *outputBuffer,
                                            unsigned long framesPerBuffer,
                                            const PaStreamCallbackTimeInfo *timeInfo,
                                            PaStreamCallbackFlags statusFlags)
{
    // since we eventually going to encode data and transfer in compressed chunks,
    // we will only tranfer data if it fully fits available space
    // since framesPerBuffer < opusFrameSize(120) we need to store the content of the buffer
    // until we have enough opusFrameSize content to start encoding
    // then we transfer the encoded data for consumption to tranferBuffer

    // 1. store new frames in intermediate ringbuffer
    // 2. if we have enough frames to encode, do the encoding into temp data
    // 3. calculate size requirements for poaDeviceData structure
    //    - if available ringbuffer size is not enough, skip writing
    //      (but log issue)
    // 5. increase opusSequenceNumber
    // 6. construct poaDeviceData structure
    // 7. put poaDeviceData into RingBuffer
    //    - first the poaCallbackTransferData, then the actual encoded data
    // 8. trigger opusAvailableFrame callback

    ring_buffer_size_t written;
    ring_buffer_size_t write_available;
    write_available = PaUtil_GetRingBufferWriteAvailable(&rIntermediateCallbackBuf);

    unsigned long toWriteFrames = framesPerBuffer > write_available ? write_available : framesPerBuffer;
    // TODO: put write_available amount of frames in the buffer instead of fully skipping
    if (toWriteFrames != framesPerBuffer)
    {
        log("_HandlePaStreamCallback SKIPPING recording, no room for (%d) intermediate frames\n", framesPerBuffer - write_available);
    }

    // 1. store new frames in intermediate ringbuffer
    written = PaUtil_WriteRingBuffer(&rIntermediateCallbackBuf, inputBuffer, framesPerBuffer);
    if (written != framesPerBuffer)
    {
        log("_HandlePaStreamCallback should not happen? frames available(%ld), written(%d)\n", framesPerBuffer, written);
    }

    ring_buffer_size_t read_available;
    read_available = PaUtil_GetRingBufferReadAvailable(&rIntermediateCallbackBuf);
    if (read_available >= inputData.opusMaxFrameSize)
    {
        log("_HandlePaStreamCallback: %dx opusMaxFrameSize available\n", read_available / inputData.opusMaxFrameSize);
    }

    // 2. if we have enough frames to encode, do the encoding into temp data

    return paContinue;
}

PaError poaEncodeInput::HandleOpenDeviceStream()
{
    PaError err = paNoError;

    return err;
}