import Rehearse20 = require('../lib/binding');

const instance = new Rehearse20("mr-yeoman");

let x=instance.OutputInitAndStartStream();
console.log(`OutputInitAndStartStream: ${x}`);

x = instance.DecodeDataIntoPlayback();
console.log(`DecodeDataIntoPlayback: ${x}`);

