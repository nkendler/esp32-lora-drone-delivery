/* 
  Sender code.

  Authentication using diffie-hellman key exchange.
  Encryption TBD.
  
*/
#define ESP32 1
#define DEBUG 1

#include "Arduino.h"
#include "heltec.h"
#include <Curve25519.h>
#include <RNG.h>
#include <ChaCha.h>
#include "ECE496.h"

#define BAND 915E6

int counter = 0;

void initSession();

void setup()
{
  Heltec.begin(true, true, true, true, BAND);

  delay(2000);

  ECE496::begin("LoRa Sender");

  // creates shared secret with base sender for encryption
  initSession();
}

void loop()
{
  counter++;
  ECE496::sendPacket(String(counter));
  ECE496::displayText(String("I am the base station!\n") +
                      "Sending packet: " + String(counter));
  delay(1000);
}

void initSession()
{
  // generate public/private key pair
  ECE496::generateKeys();
  ECE496::displayText("Generated keys!");
  delay(1000);

  // advertise connection with public key
  ECE496::displayText("Connecting...");
  ECE496::advertiseConnection();

  // recieve foreign public key from reciever
  ECE496::recieveClear(ECE496::f_publicKey, KEY_SIZE);
  ECE496::displayText("Connection established!");

  // generate secret shared key for encryption
  ECE496::generateSecret();
  ECE496::chacha.setKey(ECE496::sharedKey, KEY_SIZE);

  // generate IV for encryption with ChaCha
  ECE496::generateIV();
  ECE496::chacha.setIV(ECE496::IV, IV_SIZE);

  // deliver IV to recipient in cleartext
  ECE496::sendClear(ECE496::IV, IV_SIZE);
  delay(5000);
}
