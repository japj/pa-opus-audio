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
        log("_HandlePaStreamCallback: SKIPPING frames/partial playback, only (%d) available intermediate frames\n", framesPerBuffer - read_available);
    }

    ring_buffer_size_t read;
    read = PaUtil_ReadRingBuffer(&rIntermediateCallbackBuf, outputBuffer, toReadFrames);
    if (read != toReadFrames)
    {
        log("_HandlePaStreamCallback should not happen? frames available(%ld), read(%d)\n", read_available, read);
    }

    return paContinue;
}

PaError poaDecodeOutput::HandleOpenDeviceStream()
{

    return paNoError;
}

bool poaDecodeOutput::writeEncodedOpusFrame(/*int &sequence_number, */ void *data, int data_length)
{
    bool writtenOpusFrame = false;

    if (data_length != outputData.opusMaxFrameSize * outputData.sampleSize)
    {
        log("writeEncodedOpusFrame: buffer_size != opusMaxFrameSizeInBytes\n");
        // atm we no uncoded data, so buffer sizes need to match
        return writtenOpusFrame;
    }
    //int availableFrames = data_length / outputData.sampleSize;

    ring_buffer_size_t write_available;
    write_available = PaUtil_GetRingBufferWriteAvailable(&rIntermediateCallbackBuf);
    if (write_available < outputData.opusMaxFrameSize)
    {
        log("writeEncodedOpusFrame no room for full opus frame, only (%d)\n", write_available);
        return writtenOpusFrame;
    }

    ring_buffer_size_t write = PaUtil_WriteRingBuffer(&rIntermediateCallbackBuf, data, outputData.opusMaxFrameSize);
    writtenOpusFrame = (write == outputData.opusMaxFrameSize);
    return writtenOpusFrame;
}