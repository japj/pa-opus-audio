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
    
    PaStream *stream;
    /* */
public:
    paDecodeOutStream(/* args */);
    ~paDecodeOutStream();

    /* PUBLIC external API */

    /* TODO: some of these functions are here due to the move in progres and might not end up as part of the final API */
    int InitPaOutputData(PaSampleFormat sampleFormat, long bufferElements, unsigned int outputChannels, unsigned int rate);

    int ProtoOpenOutputStream(unsigned int rate, unsigned int channels, unsigned int device, PaSampleFormat sampleFormat, int framesPerBuffer);

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