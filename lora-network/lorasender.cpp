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

  ECE496::begin("LoRa Sender");

  // starts a session as a sender
  ECE496::initSession(true /* sender */);
}

void loop()
{
  counter++;
  ECE496::sendPacket(String(counter));
  ECE496::displayText(String("I am the base station!\n") +
                      "Sending packet: " + String(counter));
  delay(1000);
}
