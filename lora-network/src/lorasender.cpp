/* 
  Sender code.

  Elliptic curve diffie-hellman key exchange using Curve25519.
  Symmetric encryption using ChaCha20.
  
*/
#define ESP32 1
#define DEBUG 1

#include "lorasender.h"

#include "heltec.h"
#include "utils.h"

#define BAND 915E6

int counter = 0;

void ECE496::LoraSender::setup()
{
  Heltec.begin(true, true, true, true, BAND);

  delay(2000);

  ECE496::Utils::begin("LoRa Sender");

  // starts a session as a sender
  ECE496::Utils::initSession(true /* sender */);
}

void ECE496::LoraSender::loop()
{
  counter++;
  ECE496::Utils::sendPacket(String(counter));
  ECE496::Utils::displayText(String("I am the base station!\n") +
                      "Sending packet: " + String(counter));
  delay(1000);
}
