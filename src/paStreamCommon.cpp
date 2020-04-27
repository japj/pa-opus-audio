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
    PaError err;
    /* PortAudio setup*/
    err = Pa_Initialize();
	if (err != paNoError)
	{
		printf("PortAudio error: %s \n", Pa_GetErrorText(err));
		return -1;
	}
    return 0;
}

//TODO: teardownPa

int protoring()
{
	PaError err;
    unsigned int rate = 48000; //48000 enables opus enc/decoding, but some devices are 44100 which results in resampling + bigger input latency
    PaSampleFormat sampleFormat = paInt16; //paFloat32 or paInt16;
    long bufferElements = 4096; // TODO: calculate optimal ringbuffer size
    unsigned int inputChannels = 1;
    unsigned int outputChannels = 1; // since we are outputting the same data from input to output, needs to be the same channels now !
    int opusMaxFrameSize = 120; // 2.5ms@48kHz number of samples per channel in the input signal
    int paCallbackFramesPerBuffer = 64; /* since opus decodes 120 frames, this is closests to how our latency is going to be
                                        // frames per buffer for OS Audio buffer*/
    
    setupPa();

    PaDeviceIndex  outputDevice = Pa_GetDefaultOutputDevice();
    PaDeviceIndex  inputDevice  = Pa_GetDefaultInputDevice();

    paDecodeOutStream *outStream = new paDecodeOutStream();

    err = outStream->InitPaOutputData(sampleFormat, bufferElements, outputChannels, rate);
    if (err != 0){
        printf("InitPaOutputData error\n");
        return -1;
    }

    /* input stream prepare */
    PaStream *inputStream;

    paInputData *inputData = InitPaInputData(sampleFormat, bufferElements, inputChannels, rate);
    if (inputData == NULL){
        printf("InitPaInputData error\n");
        return -1;
    }

    /* record/play transfer buffer */
    long transferElementCount = bufferElements;
    long transferSampleSize = Pa_GetSampleSize(sampleFormat);
    long bufferSize = transferSampleSize * transferElementCount;
    void *transferBuffer = ALLIGNEDMALLOC(bufferSize);
    void *opusEncodeBuffer = ALLIGNEDMALLOC(bufferSize);
    void *opusDecodeBuffer = ALLIGNEDMALLOC(bufferSize);

    /* setup output device and stream */
 
    err = outStream->ProtoOpenOutputStream(rate, outputChannels, outputDevice, sampleFormat, paCallbackFramesPerBuffer);
    CHK("ProtoOpenOutputStream", err);

    printf("Pa_StartStream Output\n");
    err = outStream->StartStream();
    CHK("Pa_StartStream Output", err);

    /* setup input device and stream */
    err = ProtoOpenInputStream(&inputStream, rate, inputChannels, inputDevice, inputData, sampleFormat, paCallbackFramesPerBuffer);
    CHK("ProtoOpenInputStream", err);

    printf("Pa_StartStream Input\n");
    err = Pa_StartStream(inputStream);
    CHK("Pa_StartStream Input", err);

    int inputStreamActive = 1;
    int outputStreamActive = 1;

#define DISPLAY_STATS 0

    while (inputStreamActive && outputStreamActive) {
        ring_buffer_size_t availableInInputBuffer   = PaUtil_GetRingBufferReadAvailable(&inputData->rBufFromRT);
        ring_buffer_size_t availableToOutputBuffer  = outStream->GetRingBufferWriteAvailable();

        inputStreamActive = Pa_IsStreamActive(inputStream);
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
            ring_buffer_size_t framesWritten;
            ring_buffer_size_t framesRead;
            int toWriteFrameCount;
            void *writeBufferPtr;

            framesRead = PaUtil_ReadRingBuffer(&inputData->rBufFromRT, transferBuffer, opusMaxFrameSize);

            // can only run opus encoding/decoding on 48000 samplerate
            if (rate == 48000) {
                writeBufferPtr = opusDecodeBuffer;

                // use float32 or int16 opus encoder/decoder
                if (sampleFormat == paFloat32) {
                    int encodedPacketSize =   opus_encode_float(inputData->encoder, 
                                                    (float*)transferBuffer, 
                                                    opusMaxFrameSize, 
                                                    (unsigned char *)opusEncodeBuffer, 
                                                    bufferSize);

                    toWriteFrameCount = outStream->opusDecodeFloat(
                                                    (unsigned char *)opusEncodeBuffer,
                                                    encodedPacketSize,
                                                    (float *)opusDecodeBuffer,
                                                    opusMaxFrameSize,
                                                    0); // request in-band forward error correction
                                                        // TODO: this is 1 in rx when no packet was received/lost?
                } else {
                    int encodedPacketSize =   opus_encode(inputData->encoder, 
                                                    (opus_int16*)transferBuffer, 
                                                    opusMaxFrameSize, 
                                                    (unsigned char *)opusEncodeBuffer, 
                                                    bufferSize);

                    toWriteFrameCount = outStream->OpusDecode(
                                                    (unsigned char *)opusEncodeBuffer,
                                                    encodedPacketSize,
                                                    (opus_int16 *)opusDecodeBuffer,
                                                    opusMaxFrameSize,
                                                    0); // request in-band forward error correction
                                                        // TODO: this is 1 in rx when no packet was received/lost?   
                }
                
            }
            else {
                writeBufferPtr = transferBuffer;
                toWriteFrameCount = framesRead;
            }
         
            framesWritten = outStream->WriteRingBuffer(writeBufferPtr, toWriteFrameCount);

#if DISPLAY_STATS
            printf("In->Output availableInInputBuffer: %5d, encodedPacketSize: %5d, toWriteFrameCount: %5d, framesWritten: %5d\n", 
                                availableInInputBuffer, 
                                encodedPacketSize,
                                toWriteFrameCount,
                                framesWritten);
#endif

            availableInInputBuffer   = PaUtil_GetRingBufferReadAvailable(&inputData->rBufFromRT); 
            availableToOutputBuffer  = outStream->GetRingBufferWriteAvailable();
        }

        usleep(500);
    }

    printf("Pa_StopStream Output\n");
    err = outStream->StopStream();
    CHK("Pa_StopStream Output", err);

    printf("Pa_StopStream Input\n");
    err = Pa_StopStream(inputStream);
    CHK("Pa_StopStream Input", err);

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
    CHK("Pa_Terminate", err);

    return 0;
}