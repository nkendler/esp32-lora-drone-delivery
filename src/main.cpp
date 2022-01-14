/* 
  Check the new incoming messages, and print via serialin 115200 baud rate.
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
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
  String packetinfo;
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    packetinfo = "Received packet: \n";
    // read packet
    while (LoRa.available()) {
      packetinfo += String((char)LoRa.read()) + "\n";
    }
    // print RSSI of packet
    packetinfo += "with RSSI ";
    packetinfo += String(LoRa.packetRssi());
    Heltec.DisplayText(packetinfo);
  }
  /*else {
    Heltec.DisplayText("No Packet Detected....");
  }*/

  digitalWrite(25, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                       // wait for a second
  digitalWrite(25, LOW);    // turn the LED off by making the voltage LOW
  delay(300);  
}