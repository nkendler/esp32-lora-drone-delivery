/* 
  Sender code.

  Authentication using diffie-hellman key exchange.
  Encryption TBD.
  
*/
#include "Arduino.h"
#include "heltec.h"

//on lisa's mac - port /dev/cu.usbserial-6

//#include "heltecv2.h"
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

int counter = 0;
bool receivedmsg = false;
void setReceive(int packetSize);

void setup() {
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  String printinfo;
  String packet;
  String send; //send S
  printinfo = "I am the base station! \nWaiting to connect \nto a drone\n";

  int p = 23, g = 5, s = 4; //diffie-hellman values. see documentation/security-notes
  int k; //shared (secret) key
  int R; //receiving

  Heltec.DisplayText(printinfo);
  send = String(((int)pow(g,s)) % p);

  //first while loop - trying to connect to receiver
  //by sending a packet and waiting for a reply

  //wait for packet received and interrupt loop
  //LoRa.onReceive(setReceive);
  //LoRa.receive();
  
  //LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);

  //while (!receivedmsg) {
  while (1) {
    delay(1000);
    packetSize = LoRa.parsePacket();
    if (packetSize != 0) break;
    //send packet S
    LoRa.beginPacket();
    LoRa.print(send);
    LoRa.endPacket();
    //wait and try to get another packet     
    //delay(200);
    //LoRa.receive();
    packetSize = LoRa.parsePacket();
    if (packetSize != 0) break;
  }

  delay(2000);

  //should have received first packet (R)
  while (LoRa.available()) {
      packet += String((char)LoRa.read());
  }

  printinfo = "I am the base station! \nGot value: \n" + packet;
  Heltec.DisplayText(printinfo);

  R = packet.toInt();
  k = ((int)pow(R,s)) % p;

  printinfo = "I am the base station! \nSecured connection \nestablished\n";
  printinfo += "Shared key: " + String(k);
  Heltec.DisplayText(printinfo);
	delay(300);
  
  while (1) {
    //send packets
    printinfo = "I am the base station! \nSending packet: " + String(counter);
    LoRa.beginPacket();
    //LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(counter);
    LoRa.endPacket();
    counter++;
    Heltec.DisplayText(printinfo);
    delay(1000);
  }
}

void setReceive(int packetSize) {
  receivedmsg = true;
  return;
}
