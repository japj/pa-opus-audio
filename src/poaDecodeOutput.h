#ifndef POA_DECODE_OUTPUT_H
#define POA_DECODE_OUTPUT_H

#include "poaBase.h"

class poaDecodeOutput : public poaBase
{
private:
    /* data */
    bool isWriteEncodedOpusFrameCalled;

    OpusDecoder *decoder;
    void *opusDecodeBuffer;
    int opusDecodeBufferSize; //Number of samples per channel of available space in pcm.
                              // If this is less than the maximum packet duration (120ms; 5760 for 48kHz), this function will not be capable of decoding some packets.

    /* Packets must be passed into the decoder serially and in the correct order for a correct decode. Lost packets can be replaced with loss concealment by calling the decoder with a null pointer and zero length for the missing packet.
       In the case of PLC (data==NULL) or FEC (decode_fec=1), then frame_size needs to be exactly the duration of audio that is missing
    */
    int opusDecodeFloat(
        const unsigned char *data,
        opus_int32 len,
        float *pcm,
        int frame_size,
        int decode_fec);

    /** Decode an Opus packet
  * @param [in] data <tt>char*</tt>: Input payload. Use a NULL pointer to indicate packet loss
  * @param [in] len <tt>opus_int32</tt>: Number of bytes in payload*
  * @param [out] pcm <tt>opus_int16*</tt>: Output signal (interleaved if 2 channels). length
  *  is frame_size*channels*sizeof(opus_int16)
  * @param [in] frame_size Number of samples per channel of available space in \a pcm.
  *  If this is less than the maximum packet duration (120ms; 5760 for 48kHz), this function will
  *  not be capable of decoding some packets. In the case of PLC (data==NULL) or FEC (decode_fec=1),
  *  then frame_size needs to be exactly the duration of audio that is missing, otherwise the
  *  decoder will not be in the optimal state to decode the next incoming packet. For the PLC and
  *  FEC cases, frame_size <b>must</b> be a multiple of 2.5 ms.
  * @param [in] decode_fec <tt>int</tt>: Flag (0 or 1) to request that any in-band forward error correction data be
  *  decoded. If no such data is available, the frame is decoded as if it were lost.
  * @returns Number of decoded samples or @ref opus_errorcodes
  */
    int OpusDecode(
        const unsigned char *data,
        opus_int32 len,
        opus_int16 *pcm,
        int frame_size,
        int decode_fec);

    void DecodeOpusFrameFromTransfer();

protected:
    virtual int HandleOpenDeviceStream() override;

public:
    /** Construct poaDecodeOutput.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaDecodeOutput(const char *name);
    ~poaDecodeOutput();

    bool writeEncodedOpusFrame(poaCallbackTransferData *data);

    virtual int _HandlePaStreamCallback(const void *inputBuffer,
                                        void *outputBuffer,
                                        unsigned long framesPerBuffer,
                                        const PaStreamCallbackTimeInfo *timeInfo,
                                        PaStreamCallbackFlags statusFlags) override;
};

#endif