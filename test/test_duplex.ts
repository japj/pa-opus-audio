import Rehearse20 = require('../lib/binding');

const instance = new Rehearse20("mr-yeoman");

instance.OutputInitAndStartStream();
instance.InputInitAndStartStream();

let total=0;

function myFunc(arg:Rehearse20) {
    var b = arg.EncodeRecordingIntoData();
    if (b)
    {
        total+=b.byteLength;
        console.log(`got buffer size ${b.byteLength}, total: ${total}`);
        arg.DecodeDataIntoPlayback(b);
    }
    else{
        console.log(`got empty data, total was ${total}`);
    }
    
}

// TODO: recoording seems to work, for a limited amount of time and then it stops

let timer = setInterval( myFunc, 
            2, // every 2 ms
            instance);

console.log('Welcome to My Console,');
setTimeout(function() {
    console.log('Blah blah blah blah extra-blah');
}, 3000000);