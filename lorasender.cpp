/* 
  Basic test program, send date at the BAND you seted.
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include "Arduino.h"
#include "heltec.h"

//on lisa's mac - port /dev/cu.usbserial-6

//#include "heltecv2.h"
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

int counter = 0;

void setup() {
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Serial.println(int(Heltec.display));
  Heltec.DisplayText("First Line of Text");
  Serial.println("Cleared Display");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  String printinfo;
  String packet;
  String send;
  printinfo = "I am the base station! \nWaiting to connect to a drone\n";

  int p = 23, g = 5, s = 4; //diffie-hellman values. see documentation/security-notes
  int k; //shared (secret) key
  int R;

  Heltec.DisplayText(printinfo);
  send = String(((int)pow(g,s)) % p);
  //first while loop - trying to connect to receiver
  //by sending a packet and waiting for a reply
  while (!packetSize) {
    //send packet S
    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(send);
    LoRa.endPacket();
    delay(300); 
    //wait and try to get another packet
    packetSize = LoRa.parsePacket();
  }

  //should have received first packet (R)
  while (LoRa.available()) {
      packet += String((char)LoRa.read());
  }

  printinfo = "I am the base station! \nGot value: \n" + packet;
  Heltec.DisplayText(printinfo);

  R = packet.toInt();
  k = ((int)pow(R,s)) % p;

  printinfo = "I am the base station! \nSecured connection established\n";
  printinfo += "Shared key: " + String(k);
  Heltec.DisplayText(printinfo);
	delay(300);
  
  while (1) {
    //send packets
    printinfo = "I am the base station! \nSending packet: " + counter + "\n";
    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.print(counter);
    LoRa.endPacket();
    counter++;
    Heltec.DisplayText(printinfo);
    delay(1000);
  }
}