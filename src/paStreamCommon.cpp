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

class HandleRecordingCallback
{
    // callback handler that is called from paEncodeInstream to notify that an opus frame is available
    static void callbackHandler(void *userData)
    {
        HandleRecordingCallback *p = (HandleRecordingCallback *)userData;
        p->HandleOneOpusFrameAvailable();
    }

public:
    HandleRecordingCallback(paEncodeInStream *input, paDecodeOutStream *output)
    {
        in = input;
        out = output;
        bufferSize = in->GetMaxEncodingBufferSize();
        transferBuffer = ALLIGNEDMALLOC(bufferSize);
        cbCount = 0;
        cbCountErrors = 0;
        totalEncodedPacketSize = 0;
        totalFramesWritten = 0;

        // register callback handler
        in->setUserCallbackOpusFrameAvailable(callbackHandler, this);
    }

    void HandleOneOpusFrameAvailable()
    {
        cbCount++;
        ring_buffer_size_t framesWritten = 0;
        pAudioInputTiming timing;
        int encodedPacketSize = in->EncodeRecordingIntoData(transferBuffer, bufferSize, &timing);

        if (encodedPacketSize > 0)
        {
            //printf("going to DecodeDataIntoPlayback: %d\n", encodedPacketSize);
            // decode audio
            framesWritten = out->DecodeDataIntoPlayback(transferBuffer, encodedPacketSize);

            totalFramesWritten += framesWritten;
            totalEncodedPacketSize += encodedPacketSize;
        }
        else
        {
            cbCountErrors++;
        }

        // this should happen once every second due to 2.5ms frame
        if (cbCount % 400 == 0)
        {
            // error
            printf("HandleOneOpusFrameAvailable cbCount: %5d, cbErrors: %5d, framesWritten:%5d, encodedSize:%5d \n",
                   cbCount, cbCountErrors, totalFramesWritten, totalEncodedPacketSize);
            printf("seq: %ld, currentTime:%5f, timeStamp: %5f\n", timing.sequence_number, timing.currentTime, timing.timeStamp);
        }
    }

private:
    paEncodeInStream *in;
    paDecodeOutStream *out;
    int bufferSize;
    void *transferBuffer;
    int cbCount;
    int cbCountErrors;
    int totalEncodedPacketSize;
    int totalFramesWritten;
};

int protoring()
{
    PaError err;

    /* setup in/out devices */
    paDecodeOutStream *outStream = new paDecodeOutStream();
    err = outStream->InitForDevice();
    PaCHK("outStream->InitForDevice", err);

    paEncodeInStream *inStream = new paEncodeInStream();
    err = inStream->InitForDevice();
    PaCHK("ProtoOpenInputStream", err);

    // setup callback handler
    HandleRecordingCallback handler(inStream, outStream);

    /* start out/in streams */
    printf("Pa_StartStream Output\n");
    err = outStream->StartStream();
    PaCHK("Pa_StartStream Output", err);

    printf("Pa_StartStream Input\n");
    err = inStream->StartStream();
    PaCHK("Pa_StartStream Input", err);

    int inputStreamActive = 1;
    int outputStreamActive = 1;
    while (inputStreamActive && outputStreamActive)
    {
        ring_buffer_size_t availableInInputBuffer = inStream->GetRingBufferReadAvailable();
        ring_buffer_size_t availableToOutputBuffer = outStream->GetRingBufferWriteAvailable();
        ring_buffer_size_t availableTimingData = inStream->GetTimingDataReadAvailable();
        ring_buffer_size_t availableTimingWrite = inStream->GetTimingDataWriteAvailable();

        inputStreamActive = inStream->IsStreamActive();
        outputStreamActive = outStream->IsStreamActive();

        printf("inputActive: %5d, availableIn: %5d, availableTiming: %5d timingWrite: %5d ", inputStreamActive, availableInInputBuffer, availableTimingData, availableTimingWrite);
        printf("outputActive: %5d, availableOutput: %5d", outputStreamActive, availableToOutputBuffer);
        printf("\n");

        sleep(1);
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