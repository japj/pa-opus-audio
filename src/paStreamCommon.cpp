/*
 * prototyping portaudio ringbuffer with opus
 */
#include <stdio.h>
#include "paStreamCommon.h"
#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds

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
        bufferSize = in->GetUncompressedBufferSizeBytes();
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
        int encodedPacketSize = in->EncodeRecordingIntoData(transferBuffer, bufferSize);

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

        inputStreamActive = inStream->IsStreamActive();
        outputStreamActive = outStream->IsStreamActive();

        printf("inputStreamActive: %5d, availableInInputBuffer: %5d ", inputStreamActive, (int)availableInInputBuffer);
        printf("outputStreamActive: %5d, availableInOutputBuffer: %5d", outputStreamActive, (int)availableToOutputBuffer);
        printf("\n");

        std::this_thread::sleep_for(std::chrono::seconds(1));
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

int calcSizeUpPow2(unsigned int v)
{ // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}