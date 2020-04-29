import Rehearse20 = require('../lib/binding');

const instance = new Rehearse20("mr-yeoman");

let contentCount = 0;
let emptyCount = 0;
let totalLength = 0;

instance.SetEncodedFrameAvailableCallBack(function (b: Buffer) {
    if (b) {
        contentCount++;
        totalLength += b.byteLength;

        instance.DecodeDataIntoPlayback(b);

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

instance.OutputInitAndStartStream();
instance.InputInitAndStartStream();

console.log('Welcome to My Console,');
setTimeout(function () {
    console.log('Blah blah blah blah extra-blah');
}, 3000000);