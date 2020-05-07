#include <string.h>
#include "poaEncodeInput.h"

poaEncodeInput::poaEncodeInput(const char *name) : poaBase(name),
                                                   userCallbackOpusFrameAvailableCb(NULL),
                                                   encoder(NULL),
                                                   opusEncodeBuffer(NULL),
                                                   opusEncodeBufferSize(4000),
                                                   opusIntermediateFrameInputBuffer(NULL),
                                                   opusIntermediateFrameInputBufferSize(0)
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

    if (userCallbackOpusFrameAvailableCb != NULL)
    { // only store data if a user callback for processing is installed
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
            //log("_HandlePaStreamCallback: %dx opusMaxFrameSize available\n", read_available / inputData.opusMaxFrameSize);

            // TODO: move this to trigger with encoded data
            if (userCallbackOpusFrameAvailableCb != NULL)
            {
                //log("userCallbackOpusFrameAvailableCb\n");

                EncodeOpusFrameFromIntermediate();

                userCallbackOpusFrameAvailableCb(userCallbackOpusFrameAvailableData);
            }
        }
    }

    // 2. if we have enough frames to encode, do the encoding into temp data

    return paContinue;
}

int poaEncodeInput::HandleOpenDeviceStream()
{
    int err = 0;

    this->encoder = opus_encoder_create(inputData.sampleRate,
                                        inputData.streamParams.channelCount,
                                        OPUS_APPLICATION_RESTRICTED_LOWDELAY,
                                        &err);
    OpusLOGERR(err, "opus_encoder_create");

    opusEncodeBuffer = AllocateMemory(opusEncodeBufferSize);
    opusIntermediateFrameInputBufferSize = inputData.opusMaxFrameSize * inputData.streamParams.channelCount * inputData.sampleSize;
    opusIntermediateFrameInputBuffer = AllocateMemory(opusIntermediateFrameInputBufferSize);
    return err;
}

void poaEncodeInput::registerOpusFrameAvailableCb(paEncodeInputOpusFrameAvailableCb cb, void *userData)
{
    //log("registerOpusFrameAvailableCb\n");
    this->userCallbackOpusFrameAvailableCb = cb;
    this->userCallbackOpusFrameAvailableData = userData;
}

int poaEncodeInput::opusEncodeFloat(
    const float *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes)
{
    // TODO: The length of the encoded packet (in bytes) on success or a
    //          negative error code (see @ref opus_errorcodes) on failure.
    return opus_encode_float(this->encoder,
                             pcm,
                             frame_size,
                             data,
                             max_data_bytes);
}

int poaEncodeInput::OpusEncode(
    const opus_int16 *pcm,
    int frame_size,
    unsigned char *data,
    opus_int32 max_data_bytes)
{
    // TODO: The length of the encoded packet (in bytes) on success or a
    //          negative error code (see @ref opus_errorcodes) on failure.
    return opus_encode(this->encoder,
                       pcm,
                       frame_size,
                       data,
                       max_data_bytes);
}

void poaEncodeInput::EncodeOpusFrameFromIntermediate()
{
    int encodedPacketSize = 0;
    // clear memoty first to ensure we don't have any trash in case if partial frames
    memset(opusIntermediateFrameInputBuffer, 0, opusIntermediateFrameInputBufferSize);

    // read data from intermediate buffer so we can encode it
    ring_buffer_size_t readFrames;

    readFrames = PaUtil_ReadRingBuffer(&rIntermediateCallbackBuf, opusIntermediateFrameInputBuffer, inputData.opusMaxFrameSize);
    if (readFrames != inputData.opusMaxFrameSize)
    {
        log("EncodeOpusFrameFromIntermediate could not read full opus frame, only (%d) frames\n", readFrames);
    }

    if (inputData.streamParams.sampleFormat == paFloat32)
    {
        // encode audio
        encodedPacketSize = this->opusEncodeFloat(
            (float *)opusIntermediateFrameInputBuffer, //this->opusEncodeBuffer,
            inputData.opusMaxFrameSize,
            (unsigned char *)this->opusEncodeBuffer,
            opusEncodeBufferSize);
    }
    else
    {
        // encode audio
        encodedPacketSize = this->OpusEncode(
            (opus_int16 *)opusIntermediateFrameInputBuffer, //this->opusEncodeBuffer,
            inputData.opusMaxFrameSize,
            (unsigned char *)this->opusEncodeBuffer,
            opusEncodeBufferSize);
    }

    poaCallbackTransferData tData;
    memcpy(tData.data, opusEncodeBuffer, encodedPacketSize);
    tData.dataLength = encodedPacketSize;
    tData.sequenceNumber = opusSequenceNumber++;

    ring_buffer_size_t written = PaUtil_WriteRingBuffer(&rTransferDataBuf, &tData, 1);
    if (written != 1)
    {
        log("EncodeOpusFrameFromIntermediate FAILED PaUtil_WriteRingBuffer rTransferDataBuf for sequenceNumber(%d)\n", tData.sequenceNumber);
    }
}

bool poaEncodeInput::encodedOpusFramesAvailable()
{
    return PaUtil_GetRingBufferReadAvailable(&rTransferDataBuf) > 1;
}

bool poaEncodeInput::readEncodedOpusFrame(poaCallbackTransferData *data)
{
    ring_buffer_size_t read = PaUtil_ReadRingBuffer(&rTransferDataBuf, data, 1);
    if (read != 1)
    {
        log("poaEncodeInput::readEncodedOpusFrame failed to read data from rTransferDataBuf\n");
        // reset any data to indicate error
        memset(data, 0, sizeof(poaCallbackTransferData));
    }

    return read == 1;
}