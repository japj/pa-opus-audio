# @japj/pa-opus-audio [![Build](https://github.com/japj/pa-opus-audio/workflows/Build/badge.svg?branch=master)](https://github.com/japj/pa-opus-audio/actions?query=workflow%3ABuild)

> pa-opus-audio is a Node.js native addon for low latency PortAudio/Opus Audio Recording and Playback.

NOTE: Work in progress

This is part of the "rehearse20" project and uses concepts and code from my [trx portaudio prototype](https://github.com/japj/trx).

## Example

```ts
import { PoaInput, PoaOutput } from '../lib/binding'; // 

const input = new PoaInput();
const output = new PoaOutput();

input.setEncodedFrameAvailableCallback(function (b: Buffer) {
    if (b) {
        output.decodeAndPlay(b);
    }
});

output.initStartPlayback();
input.initStartRecord();

console.log('Recording and Playback from default OS devices');
setTimeout(function () {
    input.stopRecord();
    output.stopPlayback();
    console.log('... long wait for exiting this program');
    process.exit(0);
}, 5000);
```

## Platform support

- Linux x64
- macOS x64
- Windows x64
