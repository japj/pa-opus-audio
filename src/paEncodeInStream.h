#ifndef _PA_ENCODE_IN_STREAM
#define _PA_ENCODE_IN_STREAM

#include "paStreamCommon.h"

/**
 * callback that gets called when a new Opus Frame is available
 */
typedef void paEncodeInStreamOpusFrameAvailableCallback(void *userData);

class paEncodeInStream
{

private:
    /* BEGIN data for record callback*/
    /* Ring buffer (FIFO) for "communicating" from audio callback */
    PaUtilRingBuffer rBufFromRT;
    void *rBufFromRTData;
    /* END data for record callback*/

    std::mutex encodeRecordingIntoDataMutex;
    /* BEGIN data for record opus encoder */
    int sampleSizeSizeBytes;
    int channels;
    OpusEncoder *encoder;
    void *opusEncodeBuffer;
    int opusMaxFrameSize;
    /* END data for record opus encoder */

    /* PaStream info */
    PaStream *stream;

    /* this is data that is needed for calculation of information and cannot change after opening stream*/
    unsigned int sampleRate;
    PaSampleFormat sampleFormat;
    long maxRingBufferSamples;     // = 4096; // TODO: calculate optimal ringbuffer size
    int paCallbackFramesPerBuffer; // = 64; /* since opus encodes 120 frames, this is closests to how our latency is going to be
                                   // frames per buffer for OS Audio buffer*/

    PaStreamParameters inputParameters;
    // don't log missing audio data before encoding has started
    bool firstEncodeCalled;
    /* */

    paEncodeInStreamOpusFrameAvailableCallback *userCallbackOpusFrameAvailable;
    void *userDataCallbackOpusFrameAvailable;
    int framesWrittenSinceLastCallback;

public:
    paEncodeInStream(/* args */);
    ~paEncodeInStream();

    /* PUBLIC external API */

    int InitForDevice(PaDeviceIndex device = paDefaultDevice);
    PaError StartStream();
    PaError IsStreamActive();
    PaError StopStream();
    PaError CloseStream();

    int EncodeRecordingIntoData(void *data, opus_int32 len);

    int GetUncompressedBufferSizeBytes();

    /* TODO: some of these functions are here due to the move in progres and might not end up as part of the final API */
    int GetRingBufferReadAvailable(); // usefull for diagnostics
    int GetOpusFullFramesReadAvailable();

    void setUserCallbackOpusFrameAvailable(paEncodeInStreamOpusFrameAvailableCallback cb, void *userData);

private:
    int opusEncodeFloat(
        const float *pcm,
        int frame_size,
        unsigned char *data,
        opus_int32 max_data_bytes);

    /** Encodes an Opus frame.
  * @param [in] pcm <tt>opus_int16*</tt>: Input signal (interleaved if 2 channels). length is frame_size*channels*sizeof(opus_int16)
  * @param [in] frame_size <tt>int</tt>: Number of samples per channel in the
  *                                      input signal.
  *                                      This must be an Opus frame size for
  *                                      the encoder's sampling rate.
  *                                      For example, at 48 kHz the permitted
  *                                      values are 120, 240, 480, 960, 1920,
  *                                      and 2880.
  *                                      Passing in a duration of less than
  *                                      10 ms (480 samples at 48 kHz) will
  *                                      prevent the encoder from using the LPC
  *                                      or hybrid modes.
  * @param [out] data <tt>unsigned char*</tt>: Output payload.
  *                                            This must contain storage for at
  *                                            least \a max_data_bytes.
  * @param [in] max_data_bytes <tt>opus_int32</tt>: Size of the allocated
  *                                                 memory for the output
  *                                                 payload. This may be
  *                                                 used to impose an upper limit on
  *                                                 the instant bitrate, but should
  *                                                 not be used as the only bitrate
  *                                                 control. Use #OPUS_SET_BITRATE to
  *                                                 control the bitrate.
  * @returns The length of the encoded packet (in bytes) on success or a
  *          negative error code (see @ref opus_errorcodes) on failure.
  */
    int OpusEncode(
        const opus_int16 *pcm,
        int frame_size,
        unsigned char *data,
        opus_int32 max_data_bytes);

    ring_buffer_size_t ReadRingBuffer(PaUtilRingBuffer *rbuf, void *data, ring_buffer_size_t elementCount);

    /* INTERNAL */

public:
    /* called by low level callback , this needs to be public else it can't access it */
    int paInputCallback(const void *inputBuffer,
                        void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags);

private:
    // internal methods that should not be called from client code, relates to move in progress
    int InitPaInputData();
    PaError ProtoOpenInputStream(PaDeviceIndex device = paDefaultDevice);
};

#endif