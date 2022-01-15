/* 
  Receiver code.

  Authentication using diffie-hellman key exchange.
  Encryption TBD.
*/
#include "Arduino.h"
#include "heltec.h"

//on lisa's mac - port /dev/cu.usbserial-0001


#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6
void setup() {
    //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);

}

void loop() {
  int packetSize = LoRa.parsePacket();
  String printinfo;
  String packet;
  String send; //send R
  printinfo = "I am the drone station!\n Waiting for a connection\n";

  int p = 23, g = 5, r = 3; //diffie-hellman values. see documentation/security-notes
  int k; //shared (secret) key
  int S; //receiving

  Heltec.DisplayText(printinfo);

  //first loop - wait for sender to initiate contact by sending packet S
  while (!packetSize) {
    delay(300); 
    //wait and try to get another packet
    packetSize = LoRa.parsePacket();
  }

  //should have received first packet (S)
  while (LoRa.available()) {
      packet += String((char)LoRa.read());
  }

  printinfo = "I am the drone station! \nGot value: \n" + packet;
  Heltec.DisplayText(printinfo);

  //send S
  send = String(((int)pow(g,r)) % p);
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
  
  for (int i = 0; i < 5; i++) {
    LoRa.beginPacket();  
    LoRa.print(send);
    LoRa.endPacket();
    delay(100);
  }

  S = packet.toInt();
  k = ((int)pow(S,r)) % p;

  printinfo = "I am the drone station! \nSecured connection \nestablished\n";
  printinfo += "Shared key: " + String(k);
  Heltec.DisplayText(printinfo);
	delay(300);

  while (1) {
    packetSize = LoRa.parsePacket();

    if (packetSize) {
      // received a packet
      printinfo = "I am the drone station! \nReceived packet: \n";
      // read packet
      while (LoRa.available()) {
        printinfo += String((char)LoRa.read());
      }
      // print RSSI of packet
      printinfo += "\nwith RSSI ";
      printinfo += String(LoRa.packetRssi());
      Heltec.DisplayText(printinfo);
    }
    delay(300);
  }
}