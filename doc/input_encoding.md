# Input Encoding Information

## PortAudio

The `paStaticInputCallback` method is registered with PortAudio and will call the relevant `paEncodeInStream::paInputCallback`.

The `paInputCallback` will write input frames into a PortAudio RingBuffer to try and decouple the callback from the rest of the handling.

The opus codec requires chunks of 120 frames to be able to work.
Currently, this means that when 120 frames are available in the RingBuffer, a callback is run to indicate `OpusFrameAvailable`.
This callback must be registered with `paEncodeInStream::setUserCallbackOpusFrameAvailable`.

The `Rehearse20` class will register the static `paEncodeInStreamOpusFrameAvailableCallback` function which in turn calls `Rehearse20::handleEncodeInStreamCallback` to handle.

`handleEncodeInStreamCallback` will perform a `tsfn.NonBlockingCall` will ensure on the main js code will be executed.

On the main js thread the static `Rehearse20::JsThreadHandleEncodeInStreamCallback` will be called, which in turn calls `Rehearse20::handleEncodeInStreamCallback` to start an `EncodeWorker`.

The `EncodeWorker` will perform the actual opus encoding of the data from the RingBuffer using the `paEncodeInStream::EncodeRecordingIntoData` method.

## Ideas

- On paInputCallback perform the actual opus encoding and put the compressed data into the RingBuffer
  This requires less data in the RingBuffer at a trade off for spending more cpu time on the audio thread.

- EncodeWorker currently only handles one opus frame at a time (causing `EncodeWorker::Execute could do (x) more opus encodings`).
  
## Opus Notes

- [OPUS_APPLICATION_RESTRICTED_LOWDELAY](https://www.opus-codec.org/docs/opus_api-1.2/group__opus__encoder.html#gaa89264fd93c9da70362a0c9b96b9ca88) configures low-delay mode that disables the speech-optimized mode in exchange for slightly reduced delay.
  This mode can only be set on an newly initialized or freshly reset encoder because it changes the codec delay.
  Only use when lowest-achievable latency is what matters most.
  Voice-optimized modes cannot be used.

- DTX disabled by default (only relevant for LPC layer)
- FEC disabled by default (only relevant for LPC layer)
- OPUS_BANDWIDTH_FULLBAND (20kHz passband) by default or OPUS_AUTO?
- OPUS_AUTO signal by default
- VBR enabled by default
- Constrained VBR by default

- Opus is a stateful codec with overlapping blocks and as a result Opus packets are not coded independently of each other. Packets must be passed into the decoder serially and in the correct order for a correct decode. Lost packets can be replaced with loss concealment by calling the decoder with a null pointer and zero length for the missing packet.

- `opus_decode` `frame_size` Number of samples per channel of available space in pcm. If this is less than the maximum packet duration (120ms; 5760 for 48kHz), this function will not be capable of decoding some packets.

- `opus_decode` In the case of PLC (data==NULL) or FEC (decode_fec=1), then frame_size needs to be exactly the duration of audio that is missing, otherwise the decoder will not be in the optimal state to decode the next incoming packet.

- `opus_decoder_create` Internally Opus stores data at 48000 Hz, so that should be the default value for Fs. However, the decoder can efficiently decode to buffers at 8, 12, 16, and 24 kHz so if for some reason the caller cannot use data at the full sample rate, or knows the compressed data doesn't use the full frequency range, it can request decoding at a reduced rate. Likewise, the decoder is capable of filling in either mono or interleaved stereo pcm buffers, at the caller's request.

- [opus_defines.h](https://www.opus-codec.org/docs/html_api-1.1.0/opus__defines_8h.html) contains additional interesting items:
  - `OPUS_SET_GAIN_REQUEST`  Scales the decoded output by a factor specified in Q8 dB units. This has a maximum range of -32768 to 32767 inclusive

- `opus_encode` `max_packet` is the maximum number of bytes that can be written in the packet (4000 bytes is recommended)

## Opus Questions

- default bitrate? based on numbers of channels and input sampling rate
- default complexity?

- use [OPUS_GET_LOOKAHEAD](https://github.com/xiph/opus/blob/4f4b11c2398e96134dc62ee794bfe33ecd6e9bd2/include/opus_defines.h#L464) to determine samples to skip from the start? (`OPUS_GET_LOOKAHEAD_REQUEST`), in case of OPUS_APPLICATION_RESTRICTED_LOWDELAY this only seems to be
48000/400 = 120, meaning the first decoded opus_frame could be skipped for playback??

    ```c
    /** Gets the total samples of delay added by the entire codec.
    * This can be queried by the encoder and then the provided number of samples can be
    * skipped on from the start of the decoder's output to provide time aligned input
    * and output. From the perspective of a decoding application the real data begins this many
    * samples late.
    *
    * The decoder contribution to this delay is identical for all decoders, but the
    * encoder portion of the delay may vary from implementation to implementation,
    * version to version, or even depend on the encoder's initial configuration.
    * Applications needing delay compensation should call this CTL rather than
    * hard-coding a value.
    * @param[out] x <tt>opus_int32 *</tt>:   Number of lookahead samples
    * @hideinitializer */
    #define OPUS_GET_LOOKAHEAD(x) OPUS_GET_LOOKAHEAD_REQUEST, __opus_check_int_ptr(x)
    ```

- LSB_DEPTH (default 24)? default 16 when using opus_encode (opus_int16), helps identifying silence and near-silence.
- prediction enabled (default)?
