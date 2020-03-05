'use strict';


const fs = require('fs');

const MAX_PACKET_SIZE = 65535;

const net = require('net');
const loginRequestPacket = Buffer.from('lampa calling, shake hand?');
const loginResponsePacket = Buffer.from('welcome lampa, hand shake!');
const bamPacket = Buffer.from('bam');
const keepAlivePacket = Buffer.from('bim');
const statusPacket = 'status';

const WEB_STATUS_FILENAME = "/var/www-data/lampa/requests/status.txt";
const WEB_LOG_FILENAME = "/var/www-data/lampa/requests/log.txt";

var receivedPacket = Buffer.allocUnsafe(MAX_PACKET_SIZE);
var sendingPacket = Buffer.allocUnsafe(MAX_PACKET_SIZE);
var program_connected = 0;

var request_file = "/var/www-data/lampa/requests/request.txt";
var plan_file = "/var/www-data/lampa/requests/newplan.txt";

function logmsg(msg)
{
    fs.appendFile(WEB_LOG_FILENAME, new Date().toISOString() + ": " + msg + "\n", function(err) { if (err) { return console.log(err); } }); 
    console.log(msg);
}

function statusmsg(msg)
{
    fs.writeFile(WEB_STATUS_FILENAME, new Date().toISOString() + ": " + msg + "\n", function(err) { if (err) { return console.log(err); } }); 
    console.log(msg);
}

function processPacket(packet, socket)
{
  if (packet.compare(loginRequestPacket) == 0)
  {
    program_connected = 1;
    sendPacket(socket, loginResponsePacket);
    logmsg("program connected");
    statusmsg("online");
  }
  else if (packet.compare(bamPacket) == 0)
  {
    logmsg("alive");
  }
  else if (packet.toString().substring(0, statusPacket.length) == statusPacket)
  {
    var smsg = packet.toString().substring(statusPacket.length + 1);
    logmsg("status " + smsg);
    statusmsg(smsg);
  }
  else logmsg("unrecognized packet " + packet);
}

function sendPacket(socket, packet)
{
  if (!program_connected) 
  {
      console.log("program not connected, will not send packet");
      return;
  }
  sendingPacket.writeInt32LE(packet.length);
  packet.copy(sendingPacket, 4);
  socket.write(sendingPacket.slice(0, 4 + packet.length), (err) => { console.log(err); } );
}

function keepAlive(socket)
{
  sendPacket(socket, keepAlivePacket);
}

var program_socket;
var connection = net.createServer((socket) => {

        console.log("connection");
        program_socket = socket;
        socket.setNoDelay(true);
        var bytesOfPacketRead = 0;
        var packetLength = MAX_PACKET_SIZE;
        var pinger;
        socket.on('data', (data) => { 
                        console.log("data");
                        var arrivedBytes = data.copy(receivedPacket, bytesOfPacketRead);
                        if ((bytesOfPacketRead < 4) && (bytesOfPacketRead + arrivedBytes >= 4))
                        {
                                packetLength = receivedPacket.readIntLE(0, 4);
                                console.log("len=" + packetLength);
                        }
                        bytesOfPacketRead += arrivedBytes;
                        while (bytesOfPacketRead > packetLength + 4)
                        {
                           console.log("packet received");
                           processPacket(receivedPacket.slice(4, 4 + packetLength), socket);
                           receivedPacket.slice(packetLength + 4).copy(receivedPacket, 0);
                           bytesOfPacketRead -= packetLength + 4;
                           if (bytesOfPacketRead >= 4)
                               packetLength = receivedPacket.readIntLE(0, 4);
                           else
                               packetLength = MAX_PACKET_SIZE;
                        }
                        if (bytesOfPacketRead == packetLength + 4)
                        {
                           console.log("packet received");
                           processPacket(receivedPacket.slice(4, 4 + packetLength), socket);
                           packetLength = MAX_PACKET_SIZE;
                           bytesOfPacketRead = 0;
                        }
        });
                        
        socket.on('close', (data) => { console.log('closed connection'); 
                                       program_connected = 0; 
                                       statusmsg('offline');
                                       logmsg('program closed connection'); });
        socket.on('end', (data) => { console.log('program disconnects'); 
                                     program_connected = 0;
                                     statusmsg('offline');
                                     logmsg('program disconnected'); });
        socket.on('error', (data) => { console.log('error on socket'); logmsg('error on socket'); });
        pinger = setInterval(function() { if (program_connected) keepAlive(socket); else clearInterval(pinger); }, 60000);
     });


statusmsg("waiting");

connection.on('listening', () => { console.log('listnening...')});
connection.on('connect', () => { console.log('connect')});
connection.listen(9877, '158.195.89.120');
console.log("connection.listen()");
                

function check_request()
{
  fs.readFile(request_file, (err,data) => {
     if (err) {
       //console.error(err);
       return;
     }
     //console.log("data: " + data.toString());

     logmsg("request " + data.toString());
     sendPacket(program_socket,data);
     fs.unlink(request_file, (err) => { console.log(err); } );
   });
}

function check_plan()
{
  fs.readFile(plan_file, (err,data) => {
     if (err) {
       //console.error(err);
       return;
     }
     console.log("data: " + data.toString());
     console.log("newplan request");
     logmsg('newplan upladed');

     sendPacket(program_socket,new Buffer('newplan:' + data.toString()));
     fs.unlink(plan_file, (err) => { console.log(err); } );
   });
}

var request_responder = setInterval(function() { 
  if (!program_connected) return; 
  check_request();
  check_plan();
  },
  500);

