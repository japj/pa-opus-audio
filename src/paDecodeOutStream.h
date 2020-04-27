#ifndef _PA_DECODE_OUT_STREAM
#define _PA_DECODE_OUT_STREAM

#include "paStreamCommon.h"

class paDecodeOutStream
{
private:
    /* BEGIN data for playback callback*/
    /*  Ring buffer (FIFO) for "communicating" towards audio callback */
    PaUtilRingBuffer    rBufToRT;
    void*               rBufToRTData;
    /* END data for playback callback*/

    /* BEGIN data for playback opus decoder */
    int                 frameSizeBytes;
    int                 channels;
    OpusDecoder*        decoder;
    /* END data for playback opus decoder */
    
    /* PaStream info */
    PaStream *stream;

    /* this is data that is needed for calculation of information and cannot change after opening stream*/
    unsigned int sampleRate;
    PaSampleFormat sampleFormat;
    long bufferElements;// = 4096; // TODO: calculate optimal ringbuffer size
    int paCallbackFramesPerBuffer;// = 64; /* since opus decodes 120 frames, this is closests to how our latency is going to be
                                        // frames per buffer for OS Audio buffer*/
    //unsigned int channels; already as part of decoder needed data;

    PaStreamParameters outputParameters;
    /* */
public:
    paDecodeOutStream(/* args */);
    ~paDecodeOutStream();

    /* PUBLIC external API */

    /* TODO: some of these functions are here due to the move in progres and might not end up as part of the final API */
    int InitPaOutputData();

    int ProtoOpenOutputStream(PaDeviceIndex device = paDefaultDevice);

    int GetRingBufferWriteAvailable();

    int opusDecodeFloat(
            const unsigned char *data,
            opus_int32 len,
            float *pcm,
            int frame_size,
            int decode_fec
        );

    int OpusDecode(
            const unsigned char *data,
            opus_int32 len,
            opus_int16 *pcm,
            int frame_size,
            int decode_fec
        );

    ring_buffer_size_t WriteRingBuffer( const void *data, ring_buffer_size_t elementCount );

    PaError StartStream();
    PaError IsStreamActive();
    PaError StopStream();

    /* INTERNAL */
    /* called by low level callback , this needs to be public else it can't access it */
    int paOutputCallback( const void*                    inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags);
};



#endif