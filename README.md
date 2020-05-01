# @japj/pa-opus-audio ![Build](https://github.com/japj/pa-opus-audio/workflows/Build/badge.svg?branch=master)
>pa-opus-audio is a Node.js native addon for low latency PortAudio/Opus Audio Recording and Playback.

NOTE: Work in progress

This is part of the "rehearse20" project and uses concepts and code from my [trx portaudio prototype](https://github.com/japj/trx).

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
