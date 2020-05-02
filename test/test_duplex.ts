import { PoaInput, PoaOutput } from '../lib/binding';

const input = new PoaInput();
const output = new PoaOutput();

let contentCount = 0;
let emptyCount = 0;
let totalLength = 0;

input.setEncodedFrameAvailableCallback(function (b: Buffer) {
    if (b) {
        contentCount++;
        totalLength += b.byteLength;

        output.decodeAndPlay(b);

        if (contentCount % 100 == 0) {
            console.log(`contentCount: ${contentCount}, totalLength: ${totalLength}`);
        }
    }
    else {
        emptyCount++;

        if (contentCount % 100 == 0) {
            console.log(`emptyCount: ${emptyCount}`);
        }
    }
});

output.initStartPlayback();
input.initStartRecord();

console.log('Welcome to My Console,');
setTimeout(function () {
    console.log('Blah blah blah blah extra-blah');
}, 3000000);