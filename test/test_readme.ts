/**
 * This is the example from the readme
 */

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
}, 5000);