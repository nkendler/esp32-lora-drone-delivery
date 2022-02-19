/* 
  Sender code.

  Elliptic curve diffie-hellman key exchange using Curve25519.
  Symmetric encryption using ChaCha20.
  
*/
#define ESP32 1
#define DEBUG 1

#include "lorareceiver.h"

#include "heltec.h"
#include "utils.h"

#define BAND 915E6

int counter = 0;

void ECE496::LoraReciever::setup()
{
  Heltec.begin(true, true, true, true, BAND);

  delay(2000);

  ECE496::Utils::begin("LoRa Reciever");

  // starts a session as a reciever
  ECE496::Utils::initSession(false /* reciever */);
}

void ECE496::LoraReciever::loop()
{
  ECE496::Utils::awaitPacket();
  ECE496::Utils::displayText(String("I am the drone station!\n") +
                      "Recieved: " + ECE496::Utils::recievePacket() + "\n" +
                      "with RSSI: " + String(LoRa.packetRssi()));
}