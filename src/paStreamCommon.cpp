/*
 * prototyping portaudio ringbuffer with opus
 */
#include <stdio.h>
#include "paStreamCommon.h"
#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include "paDecodeOutStream.h"
#include "paEncodeInStream.h"

void paerror(const char *msg, int r)
{
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(Pa_GetErrorText(r), stderr);
    fputc('\n', stderr);
}

void log_pa_stream_info(PaStream *stream, PaStreamParameters *params)
{
    const PaDeviceInfo *deviceInfo;
    deviceInfo = Pa_GetDeviceInfo(params->device);
    const PaStreamInfo *streamInfo;
    streamInfo = Pa_GetStreamInfo(stream);

    printf("DeviceId:         %d (%s)\n", params->device, deviceInfo->name);
    printf("ChannelCount:     %d\n", params->channelCount);
    printf("SuggestedLatency: %f\n", params->suggestedLatency);
    printf("InputLatency:     %20f (%5.f samples)\n", streamInfo->inputLatency, streamInfo->inputLatency * streamInfo->sampleRate);
    printf("OutputLatency:    %20f (%5.f samples)\n", streamInfo->outputLatency, streamInfo->outputLatency * streamInfo->sampleRate);
    printf("SampleRate:       %.f\n", streamInfo->sampleRate);
}

/*
    // cleanup
        
*/

int setupPa()
{
    // only perform Pa initialization once
    static bool initDone = 0;
    if (initDone)
    {
        return 0;
    }
    PaError err;
    /* PortAudio setup*/
    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf("PortAudio error: %s \n", Pa_GetErrorText(err));
        return -1;
    }
    initDone = true;
    return 0;
}

//TODO: teardownPa

int protoring()
{
    PaError err;
    PaSampleFormat sampleFormat = paInt16; //paFloat32 or paInt16;
    long bufferElements = 4096;            // TODO: calculate optimal ringbuffer size
    int opusMaxFrameSize = 120;         // 2.5ms@48kHz number of samples per channel in the input signal

    /* record/play transfer buffer */
    long transferElementCount = bufferElements;
    long transferSampleSize = Pa_GetSampleSize(sampleFormat);
    long bufferSize = transferSampleSize * transferElementCount;
    void *transferBuffer = ALLIGNEDMALLOC(bufferSize);

    /* setup output device and stream */
    paDecodeOutStream *outStream = new paDecodeOutStream();
    err = outStream->InitForDevice();
    PaCHK("outStream->InitForDevice", err);

    printf("Pa_StartStream Output\n");
    err = outStream->StartStream();
    PaCHK("Pa_StartStream Output", err);

    /* setup input device and stream */
    paEncodeInStream *inStream = new paEncodeInStream();
    err = inStream->InitForDevice();
    PaCHK("ProtoOpenInputStream", err);

    printf("Pa_StartStream Input\n");
    err = inStream->StartStream();
    PaCHK("Pa_StartStream Input", err);

    int inputStreamActive = 1;
    int outputStreamActive = 1;

#define DISPLAY_STATS 0

    while (inputStreamActive && outputStreamActive)
    {
        ring_buffer_size_t availableInInputBuffer = inStream->GetRingBufferReadAvailable();
        ring_buffer_size_t availableToOutputBuffer = outStream->GetRingBufferWriteAvailable();

        inputStreamActive = inStream->IsStreamActive();
        outputStreamActive = outStream->IsStreamActive();
#if DISPLAY_STATS
        printf("inputStreamActive: %5d, availableInInputBuffer: %5d ", inputStreamActive, availableInInputBuffer);
        printf("outputStreamActive: %5d, availableInOutputBuffer: %5d", outputStreamActive, availableToOutputBuffer);
        printf("\n");
#endif

        // transfer from recording to playback by encode/decoding opus signals
        // loop through input buffer in chunks of opusMaxFrameSize
        while (availableInInputBuffer >= opusMaxFrameSize)
        {
            ring_buffer_size_t framesWritten=0;
            int encodedPacketSize = inStream->EncodeRecordingIntoData(transferBuffer, opusMaxFrameSize);
            
            if (encodedPacketSize > 0)
            {
                // decode audio
                framesWritten = outStream->DecodeDataIntoPlayback(transferBuffer, encodedPacketSize);
            } 
            else
            {
                // error
                printf("error EncodeRecordingIntoData packetSize:%d\n", encodedPacketSize);
            }


#if DISPLAY_STATS
            printf("In->Output availableInInputBuffer: %5d, encodedPacketSize: %5d, toWriteFrameCount: %5d, framesWritten: %5d\n", 
                                availableInInputBuffer, 
                                encodedPacketSize,
                                toWriteFrameCount,
                                framesWritten);
#endif

            availableInInputBuffer   = inStream->GetRingBufferReadAvailable(); 
            availableToOutputBuffer  = outStream->GetRingBufferWriteAvailable();
        }

        usleep(500);
    }

    printf("Pa_StopStream Output\n");
    err = outStream->StopStream();
    PaCHK("Pa_StopStream Output", err);

    printf("Pa_StopStream Input\n");
    err = inStream->StopStream();
    PaCHK("Pa_StopStream Input", err);

#if 0
    //todo: move all cleanup in seperate functions
    if (outputData->rBufToRTData) {
        free(outputData->rBufToRTData);
    }
    if (inputData.rBufFromRTData) {
        free(inputData.rBufFromRTData);
    }
    if (transferBuffer) {
        free(transferBuffer);
    }

    opus_encoder_destroy(encoder);
#endif

    err = Pa_Terminate();
    PaCHK("Pa_Terminate", err);

    return 0;
}