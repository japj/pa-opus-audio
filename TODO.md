# content moved from README.md

## latency-work

### test-duplex.js

During switching of apps / stress testing, occurences of:

- [PoaOutput]: _HandlePaStreamCallback: SKIPPING frames/partial playback, only (24) available intermediate frames
- [PoaOutput]: FAILED PaUtil_WriteRingBuffer rIntermediateCallbackBuf at expected sequenceNumber(3113)

### tryout

This does fully C++ based input/output (not sending the data through the js stack for monitoring)
(perhaps for local monitoring, the input/output handling needs to be done through C++ only)

### rtp receiver

TODO: test if rtp receiver only playback also has [PoaOutput] issues.

## old content

(this might not be fully accurate atm)

NOTE: Work in progress

Required before installation:

- ensure portaudio development packages are installed
- ensure opus development packages are installed

Current work will focus on getting this to end-to-end (recording/playback) state first before other work.

Status:

- paDecodeInStream is finished
- node example available that can playback concurrent rtp streams (tested with multiple instances of opusrtc)
- mainly developed on Mac, but intend is that it should work on Windows/Mac/Linux (latency will depend on used audio devices and drivers)

TODO after that (in random order):

- digitize all my notes into issues/etc
- setup CI/CD pipeline for building binary addon on Windows/Mac/Linux
- include opus and portaudio src to reduce dependencies or not having these installed (and incl patches to these where needed)
- add diagnostics to analyze latency issues (test_duplex with rtp stream and mic output seems to have a slightly delay after running for some time)

> Development

- git clone this repo, incl submodules
- go into vcpkg folder and run the bootstrap script
  - run `vcpkg @..\vcpkg_x64-<platform>.txt`
- run toplevel `npm install`
