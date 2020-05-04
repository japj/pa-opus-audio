#include <string.h>
#include "poaDecodeOutput.h"

poaDecodeOutput::poaDecodeOutput(const char *name) : poaBase(name),
                                                     isWriteEncodedOpusFrameCalled(false),
                                                     decoder(NULL),
                                                     opusDecodeBuffer(NULL),
                                                     opusDecodeBufferSize(0)
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
    if (isWriteEncodedOpusFrameCalled)
    {
        ring_buffer_size_t transfer_available;
        transfer_available = PaUtil_GetRingBufferWriteAvailable(&rTransferDataBuf);
        if (transfer_available >= 1)
        {
            DecodeOpusFrameFromTransfer();
        }
    }

    // TODO check with poaEncodeInput for consistency
    // TODO: what to do with sequence_numbers that have skipped content?

    // x. put decoded opus frame into intermediate ringbuffer

    // 8. store all possible frames from intermediate ringbuffer into outputBuffer
    // TODO: could
    ring_buffer_size_t read_available;
    read_available = PaUtil_GetRingBufferReadAvailable(&rIntermediateCallbackBuf);

    unsigned long toReadFrames = framesPerBuffer > read_available ? read_available : framesPerBuffer;
    if (toReadFrames != framesPerBuffer && isWriteEncodedOpusFrameCalled)
    {
        // only log this if encounter buffering issues after WriteEncodedOpusDataFrame has started
        log("_HandlePaStreamCallback: SKIPPING frames/partial playback, only (%d) available intermediate frames\n", toReadFrames);
        // TODO: think about skipping incoming frames until insync with sequenceNumber/playback time
    }

    ring_buffer_size_t read;
    read = PaUtil_ReadRingBuffer(&rIntermediateCallbackBuf, outputBuffer, toReadFrames);
    if (read != toReadFrames)
    {
        log("_HandlePaStreamCallback should not happen? frames available(%ld), read(%d)\n", read_available, read);
    }

    return paContinue;
}

int poaDecodeOutput::HandleOpenDeviceStream()
{
    int err = 0;

    this->decoder = opus_decoder_create(outputData.sampleRate, inputData.streamParams.channelCount, &err);
    OpusLOGERR(err, "opus_decoder_create");

    opusDecodeBufferSize = 5760 * outputData.streamParams.channelCount;
    opusDecodeBuffer = AllocateMemory(opusDecodeBufferSize);

    return paNoError;
}
/*
bool poaDecodeOutput::writeEncodedOpusFrame( void *data, int data_length)
{
    bool writtenOpusFrame = false;
    if (!isWriteEncodedOpusFrameCalled)
    {
        isWriteEncodedOpusFrameCalled = true;
    }

    if (data_length != outputData.opusMaxFrameSize * outputData.sampleSize)
    {
        log("writeEncodedOpusFrame: buffer_size != opusMaxFrameSizeInBytes\n");
        // atm we no uncoded data, so buffer sizes need to match
        return writtenOpusFrame;
    }
    //int availableFrames = data_length / outputData.sampleSize;

    ring_buffer_size_t dataAvailableFrames = data_length / outputData.sampleSize;

    ring_buffer_size_t write_available;
    write_available = PaUtil_GetRingBufferWriteAvailable(&rIntermediateCallbackBuf);

    ring_buffer_size_t toWrite = dataAvailableFrames > write_available ? write_available : dataAvailableFrames;

    ring_buffer_size_t written = PaUtil_WriteRingBuffer(&rIntermediateCallbackBuf, data, toWrite);
    writtenOpusFrame = (written == dataAvailableFrames);
    if (!writtenOpusFrame)
    {
        log("writeEncodedOpusFrame:FAILED to write full opusFrame, written only (%d) frames\n", written);
        log("this is usually an indication that wrong device settings are causing low latency to fail (you might hear a 'crackling' audio sound)\n");
    }
    return writtenOpusFrame;
}

*/

bool poaDecodeOutput::writeEncodedOpusFrame(poaCallbackTransferData *data)
{
    bool writtenOpusFrame = false;
    if (!isWriteEncodedOpusFrameCalled)
    {
        isWriteEncodedOpusFrameCalled = true;
    }

    ring_buffer_size_t write = PaUtil_WriteRingBuffer(&rTransferDataBuf, data, 1);
    if (write != 1)
    {
        log("poaDecodeOutput::writeEncodedOpusFrame failed to write data to rTransferDataBuf for sequence_number(%d)\n", data->sequenceNumber);
    }
    return write == 1;
}

int poaDecodeOutput::opusDecodeFloat(
    const unsigned char *data,
    opus_int32 len,
    float *pcm,
    int frame_size,
    int decode_fec)
{
    // TODO: Number of decoded samples or Error codes
    return opus_decode_float(this->decoder,
                             data,
                             len,
                             pcm,
                             frame_size,
                             0); // request in-band forward error correction
                                 // TODO: this is 1 in rx when no packet was received/lost?
}

int poaDecodeOutput::OpusDecode(
    const unsigned char *data,
    opus_int32 len,
    opus_int16 *pcm,
    int frame_size,
    int decode_fec)
{
    // TODO: Number of decoded samples or Error codes
    return opus_decode(this->decoder,
                       data,
                       len,
                       pcm,
                       frame_size,
                       0); // request in-band forward error correction
                           // TODO: this is 1 in rx when no packet was received/lost?
}

void poaDecodeOutput::DecodeOpusFrameFromTransfer()
{
    log("DecodeOpusFrameFromTransfer\n");
    int dec = 0; // TODO determine value

    // read data from tranferBuffer and decode it
    int decodedFrameCount = 0;

    // TODO: unify/reuse encode/decode functions on transferData
    poaCallbackTransferData tData;

    ring_buffer_size_t read = PaUtil_ReadRingBuffer(&rTransferDataBuf, &tData, 1);
    if (read != 1)
    {
        log("FAILED PaUtil_ReadRingBuffer rTransferDataBuf at sequence_number\n", opusSequenceNumber);

        // TODO: decode with dec 1??
        return;
    }

    if (outputData.streamParams.sampleFormat == paFloat32)
    {
        decodedFrameCount = this->opusDecodeFloat((unsigned char *)tData.data,
                                                  tData.dataLength,
                                                  (float *)this->opusDecodeBuffer,
                                                  outputData.opusMaxFrameSize,
                                                  dec);
    }
    else
    {
        // we assume paInt16;
        decodedFrameCount = this->OpusDecode((unsigned char *)tData.data,
                                             tData.dataLength,
                                             (opus_int16 *)this->opusDecodeBuffer,
                                             outputData.opusMaxFrameSize,
                                             dec);
    }

    if (decodedFrameCount != outputData.opusMaxFrameSize)
    {
        log("DecodeOpusFrameFromTransfer could not decode full opus frame, only (%d) frames\n", decodedFrameCount);
    }

    // write decoded data to the intermediate ringbuffer
    ring_buffer_size_t written = PaUtil_WriteRingBuffer(&rIntermediateCallbackBuf, opusDecodeBuffer, decodedFrameCount);
    if (written != decodedFrameCount)
    {
        log("FAILED PaUtil_WriteRingBuffer rIntermediateCallbackBuf for sequenceNumber(%d)\n", tData.sequenceNumber);
    }
}