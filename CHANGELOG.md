# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

### [Unreleased]

### Changed

- automatic adjusting of internal input/output buffers based on portaudio latency info
- automatic skipping of opus frames in case too much "running behind" detected during playback

## [0.2.1] - 2020-05-08

### Fixed

- 0.2.0 is missing lib/binding.js & typings #18
- "muting" in example results in lots of logging #19

## [0.2.0] - 2020-05-08

### Changed

- use vcpkg overlays to manage custom portaudio/opus ports (#14)
- switched back to node abi prebuilds instead of napi
- internal refactoring on opus encoding/decoding in the portaudio callback [#10]
  - includes improved logging for analysis on audio/latency issues
- osx: use paMacCoreMinimizeCPU to change device settings and minimize CPU usage
- linux(alsa): use PaAlsa_EnableRealtimeScheduling

### Fixed

- Prebuild Linux segfault due to portaudio without alsa backend configured

## [0.1.0] - 2020-05-02

Initial release to check prebuild workflow.
This uses concepts and code from my [trx portaudio prototype](https://github.com/japj/trx).
