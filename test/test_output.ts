import Rehearse20 = require('../lib/binding');
import Rtp = require('./rtp');

const instance = new Rehearse20("mr-yeoman");

let x=instance.OutputInitAndStartStream();
console.log(`OutputInitAndStartStream: ${x}`);


import udp = require('dgram');

// creating a udp server
var server = udp.createSocket('udp4');

// emits when any error occurs
server.on('error',function(error:string){
  console.log('Error: ' + error);
  server.close();
});

// emits on new datagram msg
server.on('message',function(msg: Buffer, info: udp.RemoteInfo) {
    //console.log('Data received from client : ' + msg.toString());
    console.log('Received %d bytes from %s:%d\n', msg.length, info.address, info.port);
    console.log(`version        : ${Rtp.version(msg)}`);
    console.log(`extension      : ${Rtp.extension(msg)}`);
    console.log(`payloadType    : ${Rtp.payloadType(msg)}`);
    console.log(`sequenceNumber : ${Rtp.sequenceNumber(msg)}`);
    console.log(`timestamp      : ${Rtp.timestamp(msg)}`);
    console.log(`sSrc           : ${Rtp.sSrc(msg)}`);

    let payload  = Rtp.payload(msg);
    console.log(`payload.length : ${payload.byteLength}`);
    let status = instance.DecodeDataIntoPlayback(payload);
    console.log(`status: ${status}`);
});

//emits when socket is ready and listening for datagram msgs
server.on('listening',function(){
    var address = server.address();
    var port = address.port;
    var family = address.family;
    var ipaddr = address.address;
    console.log('Server is listening at port' + port);
    console.log('Server ip :' + ipaddr);
    console.log('Server is IP4/IP6 : ' + family);
  });
  
  //emits after the socket is closed using socket.close();
  server.on('close',function(){
    console.log('Socket is closed !');
  });
  
  server.bind(2222);