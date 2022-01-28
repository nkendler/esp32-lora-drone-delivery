/* 
  Sender code.

  Elliptic curve diffie-hellman key exchange using Curve25519.
  Symmetric encryption using ChaCha20.
  
*/
#define ESP32 1
#define DEBUG 1

#include "Arduino.h"
#include "heltec.h"
#include "ECE496.h"

#define BAND 915E6

int counter = 0;

void setup()
{
  Heltec.begin(true, true, true, true, BAND);

  delay(2000);

  ECE496::begin("LoRa Reciever");

  // starts a session as a reciever
  ECE496::initSession(false /* reciever */);
}

void loop()
{
  ECE496::awaitPacket();
  ECE496::displayText(String("I am the drone station!\n") +
                      "Recieved: " + ECE496::recievePacket() + "\n" +
                      "with RSSI: " + String(LoRa.packetRssi()));
}