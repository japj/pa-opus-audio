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
    void*               opusDecodeBuffer;
    int                 opusMaxFrameSize;
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

    int InitForDevice(PaDeviceIndex device = paDefaultDevice);
    PaError StartStream();
    PaError IsStreamActive();
    PaError StopStream();

    //int dec: Flag (0 or 1) to request that any in-band forward error correction data be decoded. If no such data is available, the frame is decoded as if it were lost.
    int DecodeDataIntoPlayback(void *data, opus_int32 len, int dec = 0);

    /* TODO: some of these functions are here due to the move in progres and might not end up as part of the final API */
    int GetRingBufferWriteAvailable(); // usefull for diagnostics

private:

    /* Packets must be passed into the decoder serially and in the correct order for a correct decode. Lost packets can be replaced with loss concealment by calling the decoder with a null pointer and zero length for the missing packet.
       In the case of PLC (data==NULL) or FEC (decode_fec=1), then frame_size needs to be exactly the duration of audio that is missing
    */
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

    /* INTERNAL */

public:

    /* called by low level callback , this needs to be public else it can't access it */
    int paOutputCallback( const void*                     inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
			              const PaStreamCallbackTimeInfo* timeInfo,
			              PaStreamCallbackFlags           statusFlags);

private:

    // internal methods that should not be called from client code, relates to move in progress
    int InitPaOutputData();
    PaError ProtoOpenOutputStream(PaDeviceIndex device = paDefaultDevice);

};

#endif