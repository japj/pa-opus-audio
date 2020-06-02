#include <string.h>
#include <math.h>
#include "poaDecodeOutput.h"

poaDecodeOutput::poaDecodeOutput(const char *name) : poaBase(name),
                                                     isWriteEncodedOpusFrameCalled(false),
                                                     decoder(NULL),
                                                     opusDecodeBuffer(NULL),
                                                     opusDecodeBufferSize(0),
                                                     framesSkippedLastTime(0),
                                                     lastFramesSkippedAtSequenceNumber(0),
                                                     currentSkippedFrameCount(0)
{
    //is this needed? outputData.streamParams.channelCount = 2;

    transferDataElements = 2; // ONLY BUFFER 2 encoded frames for output
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

    if (framesPerBuffer != outputData.callbackMaxFrameSize)
    {
        log("framesPerBuffer(%d) != outputData.callbackMaxFrameSize(%d)\n", framesPerBuffer, outputData.callbackMaxFrameSize);
    }

    // 1. if we have enough space in intermediate buffer for a decoded opus_frame, start decoding
    //    - need to read a poaDeviceData structure
    if (isWriteEncodedOpusFrameCalled)
    {
        ring_buffer_size_t transfer_available;
        ring_buffer_size_t intermediate_available;
        transfer_available = PaUtil_GetRingBufferReadAvailable(&rTransferDataBuf);
        intermediate_available = PaUtil_GetRingBufferWriteAvailable(&rIntermediateCallbackBuf);

        //TODO: determine when is the point where we ask opus to generate audio based on missed packet?
        if (transfer_available >= 1 &&
            intermediate_available >= outputData.opusMaxFrameSize)
        {
            DecodeOpusFrameFromTransfer();
        }
        /*else
        {
            log("_HandlePaStreamCallback WARNING: transferavailable(%d), intermediateavailable(%d)\n",
                transfer_available,
                intermediate_available);
        }*/
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
            // TODO: think about skipping incoming frames until insync with sequenceNumber/playback time
            framesSkippedLastTime = framesPerBuffer - toReadFrames;
            currentSkippedFrameCount += (framesPerBuffer - toReadFrames);

            // only log this if encounter buffering issues after WriteEncodedOpusDataFrame has started
            if (opusSequenceNumber != lastFramesSkippedAtSequenceNumber)
            {
                // only log this ONCE for all times a skip happens for the same opusSequenceNumber to prevent excessive log spamming
                log("_HandlePaStreamCallback: SKIPPING(%d) frames/partial playback, only (%d) available intermediate frames at sequence(%d), currentSkippedFrameCount(%d)\n",
                    framesSkippedLastTime, toReadFrames, opusSequenceNumber, currentSkippedFrameCount);
            }
            lastFramesSkippedAtSequenceNumber = opusSequenceNumber;
        }
        else
        {
            framesSkippedLastTime = 0;
        }

        ring_buffer_size_t read;
        read = PaUtil_ReadRingBuffer(&rIntermediateCallbackBuf, outputBuffer, toReadFrames);
        if (read != toReadFrames)
        {
            log("_HandlePaStreamCallback should not happen? frames available(%ld), read(%d)\n", read_available, read);
        }
    }

    // if no encoded data was queued, we just output "empty" audio through the memset
    return paContinue;
}

int poaDecodeOutput::HandleOpenDeviceStream()
{
    // adjust transferDataElements size depending on outputBufferSize
    const PaStreamInfo *streamInfo;
    streamInfo = GetStreamInfo();
    double outputLatencySamples = streamInfo->outputLatency * streamInfo->sampleRate;
    if (outputLatencySamples > transferDataElements * outputData.opusMaxFrameSize)
    {
        transferDataElements = 2 * ceil(outputLatencySamples / outputData.opusMaxFrameSize);
        log("poaDecodeOutput::HandleOpenDeviceStream ADJUSTING transferDataElements to: %d\n", transferDataElements);
        log("!!! outputLatencySamples (%4.f) transferDataElements * opusMaxFrameSize (%4d) !!!\n",
            outputLatencySamples, transferDataElements * outputData.opusMaxFrameSize);
    }

    // setup decoder
    int err = 0;

    this->decoder = opus_decoder_create(outputData.sampleRate, inputData.streamParams.channelCount, &err);
    OpusLOGERR(err, "opus_decoder_create");

    opusDecodeBufferSize = 5760;
    opusDecodeBuffer = AllocateMemory(opusDecodeBufferSize * outputData.streamParams.channelCount * outputData.sampleSize);

    return paNoError;
}

bool poaDecodeOutput::writeEncodedOpusFrame(poaCallbackTransferData *data)
{
    bool writtenOpusFrame = false;

    if (!IsCallbackRunning())
    {
        log("poaDecodeOutput::writeEncodedOpusFrame SKIPPING write, since callback is not running yet\n");
        return writtenOpusFrame;
    }

    ring_buffer_size_t write = PaUtil_WriteRingBuffer(&rTransferDataBuf, data, 1);
    if (write != 1)
    {
        log("poaDecodeOutput::writeEncodedOpusFrame FAILED to write data to rTransferDataBuf for data->sequenceNumber(%5d) playbackSequenceNumber(%d), IsCallbackRunning: %d\n",
            data->sequenceNumber,
            opusSequenceNumber,
            IsCallbackRunning());
    }
    else
    {
        /*log("poaDecodeOutput::writeEncodedOpusFrame wrote to rTransferDataBuf for data->sequenceNumber(%5d) playbackSequenceNumber(%d), IsCallbackRunning: %d\n",
            data->sequenceNumber,
            opusSequenceNumber,
            IsCallbackRunning()); */
    }

    // TODO: align opusSequeneNumber and decoderSequenceNumber usage
    if (!isWriteEncodedOpusFrameCalled)
    {
        isWriteEncodedOpusFrameCalled = true;
        log("writeEncodedOpusFrame isWriteEncodedOpusFrameCalled(%d) data->sequenceNumber(%d)\n", isWriteEncodedOpusFrameCalled, data->sequenceNumber);
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
    int dec = 0; // TODO determine value

    // read data from tranferBuffer and decode it
    int decodedFrameCount = 0;

    // TODO: unify/reuse encode/decode functions on transferData
    poaCallbackTransferData tData;

    ring_buffer_size_t read = PaUtil_ReadRingBuffer(&rTransferDataBuf, &tData, 1);
    if (read != 1)
    {
        log("DecodeOpusFrameFromTransfer: No encoded data available yet PaUtil_ReadRingBuffer rTransferDataBuf at sequence_number (%d)\n", opusSequenceNumber);

        // TODO: decode with dec 1??
        return;
    }

    if (tData.sequenceNumber != opusSequenceNumber + 1)
    {
        log("DecodeOpusFrameFromTransfer expected sequenceNumber (%5d) but got (%5d) instead\n", opusSequenceNumber + 1, tData.sequenceNumber);
    }

    //log("DecodeOpusFrameFromTransfer opusSequenceNumber (%d) encoded.sequenceNumber (%d)\n", opusSequenceNumber, tData.sequenceNumber);

    if (outputData.streamParams.sampleFormat == paFloat32)
    {
        decodedFrameCount = this->opusDecodeFloat((unsigned char *)tData.data,
                                                  tData.dataLength,
                                                  (float *)this->opusDecodeBuffer,
                                                  this->opusDecodeBufferSize,
                                                  dec);
    }
    else
    {
        // we assume paInt16;
        decodedFrameCount = this->OpusDecode((unsigned char *)tData.data,
                                             tData.dataLength,
                                             (opus_int16 *)this->opusDecodeBuffer,
                                             this->opusDecodeBufferSize,
                                             dec);
    }

    if (decodedFrameCount < 0)
    {
        logOpusError(decodedFrameCount, "DecodeOpusFrameFromTransfer failed decoding opus Frame");
    }

    if (decodedFrameCount != outputData.opusMaxFrameSize)
    {
        log("DecodeOpusFrameFromTransfer could not decode full opus frame, only (%d) frames\n", decodedFrameCount);
    }

    if (decodedFrameCount > 0)
    {
        if (framesSkippedLastTime > 0)
        {
            log("DecodeOpusFrameFromTransfer: framesSkippedLastTime (%5d) currentSkippedFrameCount(%5d) at sequence(%d)\n", framesSkippedLastTime, currentSkippedFrameCount, opusSequenceNumber);
        }

        if (framesSkippedLastTime <= outputData.opusMaxFrameSize)
        {
            // write decoded data to the intermediate ringbuffer
            uint8_t *skipAdjustedBuffer = (uint8_t *)opusDecodeBuffer;
            // TODO: skipping not good? skipAdjustedBuffer += framesSkippedLastTime * outputData.sampleSize;
            ring_buffer_size_t skipAdjustedFrameCount = decodedFrameCount; //      -framesSkippedLastTime;

            ring_buffer_size_t written = PaUtil_WriteRingBuffer(&rIntermediateCallbackBuf, skipAdjustedBuffer, skipAdjustedFrameCount);
            if (written != skipAdjustedFrameCount)
            {
                log("FAILED PaUtil_WriteRingBuffer rIntermediateCallbackBuf at expected sequenceNumber(%d)\n", opusSequenceNumber + 1);
            }
        }

        // TODO if we have too many pending encoded data, perhaps skip one?
        {
            ring_buffer_size_t transfer_available;
            transfer_available = PaUtil_GetRingBufferReadAvailable(&rTransferDataBuf);
            if (transfer_available > 1)
            {
                // only log if we have more than 1 additional encoded frame pending
                log("DecodeOpusFrameFromTransfer after decoding, there is still (%d) transfer_available tData.sequenceNumber(%d) currentSkippedFrameCount(%d)\n", transfer_available, tData.sequenceNumber, currentSkippedFrameCount);

                if (currentSkippedFrameCount >= 2 * outputData.opusMaxFrameSize)
                {
                    log("currentSkippedFrameCount(%d) >= outputData.opusMaxFrameSize(%d), SKIPADJUST a full frame from audio playback\n", currentSkippedFrameCount, outputData.opusMaxFrameSize);

                    // reading into tData so expected sequenceNumber gets updated too
                    ring_buffer_size_t read = PaUtil_ReadRingBuffer(&rTransferDataBuf, &tData, 1);
                    if (read != 1)
                    {
                        log("FAILED SKIPADJUST\n");
                    }
                    else
                    {

                        currentSkippedFrameCount -= outputData.opusMaxFrameSize;
                    }
                }
            }
        }
    }

    opusSequenceNumber = tData.sequenceNumber;
}