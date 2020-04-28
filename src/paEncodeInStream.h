#ifndef _PA_ENCODE_IN_STREAM
#define _PA_ENCODE_IN_STREAM

#include "paStreamCommon.h"

class paEncodeInStream 
{

private:

    /* BEGIN data for record callback*/
    /* Ring buffer (FIFO) for "communicating" from audio callback */
    PaUtilRingBuffer    rBufFromRT;
    void*               rBufFromRTData;
    /* END data for record callback*/

    /* BEGIN data for record opus encoder */
    int                 frameSizeBytes;
    int                 channels;
    OpusEncoder*        encoder;
    void*               opusEncodeBuffer;
    int                 opusMaxFrameSize;
    /* END data for record opus encoder */

    /* PaStream info */
    PaStream *stream;

    /* this is data that is needed for calculation of information and cannot change after opening stream*/
    unsigned int sampleRate;
    PaSampleFormat sampleFormat;
    long bufferElements;// = 4096; // TODO: calculate optimal ringbuffer size
    int paCallbackFramesPerBuffer;// = 64; /* since opus encodes 120 frames, this is closests to how our latency is going to be
                                        // frames per buffer for OS Audio buffer*/

    PaStreamParameters inputParameters;
    /* */

public:

    paEncodeInStream(/* args */);
    ~paEncodeInStream();

    /* PUBLIC external API */

    int InitForDevice(PaDeviceIndex device = paDefaultDevice);
    PaError StartStream();
    PaError IsStreamActive();
    PaError StopStream();

    /* TODO: some of these functions are here due to the move in progres and might not end up as part of the final API */
    int GetRingBufferReadAvailable(); // usefull for diagnostics

private:

    int opusEncodeFloat(
        const float *pcm,
        int frame_size,
        unsigned char *data,
        opus_int32 max_data_bytes);

    int OpusEncode(
        const opus_int16 *pcm,
        int frame_size,
        unsigned char *data,
        opus_int32 max_data_bytes);
    
    ring_buffer_size_t ReadRingBuffer( PaUtilRingBuffer *rbuf, void *data, ring_buffer_size_t elementCount );

    /* INTERNAL */

public:

    /* called by low level callback , this needs to be public else it can't access it */
    int paInputCallback(  const void*                     inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags);
private:

    // internal methods that should not be called from client code, relates to move in progress
    int InitPaInputData();
    PaError ProtoOpenInputStream(PaDeviceIndex device = paDefaultDevice);

};

#endif