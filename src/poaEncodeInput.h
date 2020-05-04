#ifndef POA_ENCODE_INPUT_H
#define POA_ENCODE_INPUT_H

#include "poaBase.h"

typedef void paEncodeInputOpusFrameAvailableCb(void *userData);

class poaEncodeInput : public poaBase
{
private:
    paEncodeInputOpusFrameAvailableCb *userCallbackOpusFrameAvailableCb;
    void *userCallbackOpusFrameAvailableData;

    OpusEncoder *encoder;
    void *opusEncodeBuffer;
    size_t opusEncodeBufferSize; // `max_packet` is the maximum number of bytes that can be written in the packet(4000 bytes is recommended)
    void *opusIntermediateFrameInputBuffer;
    size_t opusIntermediateFrameInputBufferSize;

    int
    opusEncodeFloat(
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

    void EncodeOpusFrameFromIntermediate();

protected:
    virtual int HandleOpenDeviceStream() override;

public:
    /** Construct poaEncodeInput.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaEncodeInput(const char *name);
    ~poaEncodeInput();

    void registerOpusFrameAvailableCb(paEncodeInputOpusFrameAvailableCb cb, void *userData);

    int encodedOpusFramesAvailable();
    bool readEncodedOpusFrame(poaCallbackTransferData *data);

    virtual int
    _HandlePaStreamCallback(const void *inputBuffer,
                            void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags) override;
};

#endif