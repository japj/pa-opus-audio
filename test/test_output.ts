import Rehearse20 = require('../lib/binding');
import Rtp = require('./rtp');

const instance = new Rehearse20("mr-yeoman");

import udp = require('dgram');

// creating a udp server
var server = udp.createSocket('udp4');

// emits when any error occurs
server.on('error',function(error:string){
  console.log('Error: ' + error);
  server.close();
});

let rtpClients = new Map<string, Rehearse20>();

let contentCount = 0;
let emptyCount = 0;
let totalLength = 0;

/** this is for recording mic and playback on own output device so you can hear what you say */
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

// emits on new datagram msg
server.on('message',function(msg: Buffer, info: udp.RemoteInfo) {
    let clientKey = `${info.address}:${info.port}`;
    let payload  = Rtp.payload(msg);

    //console.log(`'${clientKey}' send msg with length ${msg.length}`);
    if (!rtpClients.has(clientKey)) {
      console.log(`NEW client detected at: ${clientKey}`);
      let rtpClient = new Rehearse20(clientKey);
      let x = rtpClient.OutputInitAndStartStream();
      console.log(`OutputInitAndStartStream: ${x}`);
      rtpClients.set(clientKey, rtpClient);
    }
    let currentClient = rtpClients.get(clientKey);
    if (currentClient) {
      let status = currentClient.DecodeDataIntoPlayback(payload);
    }

    /*
    console.log(`version        : ${Rtp.version(msg)}`);
    console.log(`extension      : ${Rtp.extension(msg)}`);
    console.log(`payloadType    : ${Rtp.payloadType(msg)}`);
    console.log(`sequenceNumber : ${Rtp.sequenceNumber(msg)}`);
    console.log(`timestamp      : ${Rtp.timestamp(msg)}`);
    console.log(`sSrc           : ${Rtp.sSrc(msg)}`);
    console.log(`payload.length : ${payload.byteLength}`);
    */
    //console.log(`status: ${status}`);
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