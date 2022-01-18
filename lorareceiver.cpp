/* 
  Sender code.

  Authentication using diffie-hellman key exchange.
  Encryption TBD.
  
*/
#define ESP32 1

#include "Arduino.h"
#include "heltec.h"
#include <Curve25519.h>
#include <RNG.h>
#include <ChaCha.h>
#include "ECE496.h"

#define DEBUG 1
#define BAND 915E6

int counter = 0;

void initSession();

void setup()
{
  Heltec.begin(true, true, true, true, BAND);

  delay(2000);

  ECE496::begin("LoRa Reciever");

  // creates shared secret with base sender for encryption
  initSession();
}

void loop()
{
  ECE496::awaitPacket();
  ECE496::displayText(String("I am the drone station!\n") +
                      "Recieved: " + ECE496::recievePacket() + "\n" +
                      "with RSSI: " + String(LoRa.packetRssi()));
}

void initSession()
{
  // check if there is sufficient entropy stored
  ECE496::allocateEntropy(KEY_SIZE);

  // generate public/private key pair
  ECE496::generateKeys();
  ECE496::displayText("Generated keys!");
  delay(1000);

  // await advertisement from sender
  ECE496::displayText("Connecting...");
  ECE496::awaitPacket();

  // recieve foreign public key from sender
  ECE496::recieveClear(ECE496::f_publicKey, KEY_SIZE);

  // reply with our public key for generating our shared secret
  ECE496::sendClear(ECE496::publicKey, KEY_SIZE);
  ECE496::displayText("Connection established!");

  // generate secret shared key for encryption
  ECE496::generateSecret();
  ECE496::chacha.setKey(ECE496::sharedKey, KEY_SIZE);

  // recieve IV/Nonce from sender to be used in our ChaCha20
  ECE496::awaitPacket();
  ECE496::recieveClear(ECE496::IV, IV_SIZE);
  ECE496::chacha.setIV(ECE496::IV, IV_SIZE);
  ECE496::logHex("IV: ", ECE496::IV, IV_SIZE);
}
