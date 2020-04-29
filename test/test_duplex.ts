import Rehearse20 = require('../lib/binding');

const instance = new Rehearse20("mr-yeoman");

instance.OutputInitAndStartStream();
instance.InputInitAndStartStream();

let contentCount = 0;
let emptyCount = 0;

let totalLength = 0;

function myFunc(arg: Rehearse20) {

    var b = arg.EncodeRecordingIntoData();
    if (b) {
        contentCount++;
        totalLength += b.byteLength;

        arg.DecodeDataIntoPlayback(b);

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
}

let useTimer = false; // use timer or while loop for record/playback

if (useTimer)
{
    // duplex record/playback with timer does not give good audio quality/latency (as expected)
    let timer = setInterval(myFunc,
        1, // every 2 ms
        instance);
}
else
{
    // duplex record/playback sounds perfect, however due to 'polling' it has a high cpu usage
    while (true) {
        myFunc(instance);
    }
}

console.log('Welcome to My Console,');
setTimeout(function () {
    console.log('Blah blah blah blah extra-blah');
}, 3000000);