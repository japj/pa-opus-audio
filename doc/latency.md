# pa-opus-audio latency information

## assumptions

- focus on achieving low latency

  - if we need to drop input data because consumer can't keep up we will actually drop data
  - if we need to drop output data because producer can't keep up we will actually drop data
  - opus 2.5ms@48kHz means a frame_size of 120 frames
    - this means we need to wait for 120 frames to be available from input before start encoding
    - this means we will receive in chunks of 120 frames
      - playback can start at the earliest 2.5ms after recording is done
  - portaudio provides PaStreamInfo after stream is opened
    - inputLatency
    - outputLatency
  - portaudio OpenStream framesPerBuffer: the number of frames passed to the stream callback
  - output.framesPerBuffer determines the earliest when playback can start

## Example

    DeviceId:         1 (Built-in Output)
    ChannelCount:     1
    SuggestedLatency: 0.003625
    InputLatency:                 0.000000 (    0 samples)
    OutputLatency:                0.003625 (  174 samples)
    SampleRate:       48000

    DeviceId:         0 (Built-in Microphone)
    ChannelCount:     1
    SuggestedLatency: 0.002771
    InputLatency:                 0.002771 (  133 samples)
    OutputLatency:                0.000000 (    0 samples)
    SampleRate:       48000

    input callback wil be called at the earliest after 133 samples of recording

    133: can encode one opus chunk (120 frames)

    when is the callback function first called?
    - input.framesPerBuffer < 133
    - input.framesPerBuffer > 133

## Ideas

- input ringbuffer is
  - minimal 120 opusframe
  - ??max(160, PaStreamInfo.inputLatency) + input.framesPerBuffer?
- input.framesPerBuffer = 64/128/256/512?

input (latency 133)
120 -> 120

64 -> 64
64 -> 128 -> -120 -> 8
64 -> 72
64 -> 

output latency

2 * opus frame ?
