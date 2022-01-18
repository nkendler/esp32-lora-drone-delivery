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
#include "esp_wifi.h"
#include <ChaCha.h>

//on lisa's mac - port /dev/cu.usbserial-6

#define IV_SIZE 8
uint8_t IV[IV_SIZE];

#define KEY_SIZE 32
uint8_t noise[32];

uint8_t publicKey[KEY_SIZE];  // our public key
uint8_t privateKey[KEY_SIZE]; // our private key

uint8_t f_publicKey[KEY_SIZE]; // foreign public key
uint8_t sharedKey[KEY_SIZE];   // shared secret

// encryption core
ChaCha chacha = ChaCha();

#define DEBUG 1
#define BAND 915E6

int counter = 0;

void allocateEntropy(size_t size);
void initSession();
void advertiseConnection();
void awaitPacket();
String recievePacket();
void sendPacket(String s);
void displayText(String s);
void generateSecret();
void logHex(String n, uint8_t *s, size_t size);
void generateKeys();
void generateIV();
void sendClear(uint8_t *buf, size_t size);
void sendCipher(uint8_t *buf, size_t size);
void encrypt(uint8_t *input, size_t size);
void decrypt(uint8_t *input, size_t size);
void recieveClear(uint8_t *buf, size_t size);
void recieveCipher(uint8_t *buf, size_t size);

void setup()
{
  Heltec.begin(true, true, true, true, BAND);

  delay(2000);

  // generate sufficient entropy for our uses
  RNG.begin("LoRa Reciever");

  // creates shared secret with base sender for encryption
  initSession();
}

void loop()
{
  awaitPacket();
  displayText(String("I am the drone station!\n") +
              "Recieved: " + recievePacket() + "\n" +
              "with RSSI: " + String(LoRa.packetRssi()));
}

// generate public-private key pair using ECDH
void generateKeys()
{
  allocateEntropy(KEY_SIZE);
  Curve25519::dh1(publicKey, privateKey);
  logHex("Public key: ", publicKey, KEY_SIZE);
  logHex("Private key: ", privateKey, KEY_SIZE);
}

// log a buffer of bytes to the Serial
void logHex(String n, uint8_t *s, size_t size)
{
  if (!DEBUG)
    return;
  Serial.print(n);
  for (size_t i = 0; i < size; i++)
  {
    Serial.printf("%02x", *(s + i));
  }
  Serial.println();
}

// generate shared secret from ECDH
void generateSecret()
{
  Curve25519::dh2(f_publicKey, privateKey);
  memcpy(sharedKey, f_publicKey, KEY_SIZE);
  logHex("Shared secret: ", sharedKey, KEY_SIZE);
}

// display text on the OLED display, and log it to Serial if we're in DEBUG mode
void displayText(String s)
{
  Heltec.DisplayText(s);
  if (DEBUG)
    Serial.print(s + "\n");
}

// send an encrypted packet
void sendPacket(String s)
{
  // convert the input String to an array of bytes
  uint8_t message[s.length() + 1];
  s.toCharArray((char *)message, s.length() + 1);

  // send the message to the recipient in ciphertext
  sendCipher(message, s.length() + 1);
}

// randomly generate IV/nonce for use in ChaCha20
void generateIV()
{
  // check if there is enough entropy in the system for IV
  allocateEntropy(IV_SIZE);

  // randomly generate IV
  RNG.rand(IV, IV_SIZE);
  logHex("IV: ", IV, IV_SIZE);
}

// send a cleartext message
void sendClear(uint8_t *buf, size_t size)
{
  LoRa.beginPacket();
  LoRa.write(buf, size);
  LoRa.endPacket();
}

// encrypt a cleartext message into ciphertext and then send it
void sendCipher(uint8_t *buf, size_t size)
{
  encrypt(buf, size);
  LoRa.beginPacket();
  LoRa.write(buf, size);
  LoRa.endPacket();
}

// encrypt a buffer
void encrypt(uint8_t *input, size_t size)
{
  chacha.encrypt(input, input, size);
}

// decrypt a buffer
void decrypt(uint8_t *input, size_t size)
{
  chacha.decrypt(input, input, size);
}

// recieve an encrypted packet
String recievePacket()
{
  // recieve ciphertext message and decrypt it to cleartext
  uint8_t coded[LoRa.available()];
  recieveCipher(coded, LoRa.available());

  // return message as a String
  return String((char *)coded);
}

// read cleartext message
void recieveClear(uint8_t *buf, size_t size)
{
  LoRa.readBytes(buf, size);
  logHex("Recieved clear: ", buf, size);
}

// read ciphertext message and decrypt it into cleartext
void recieveCipher(uint8_t *buf, size_t size)
{
  LoRa.readBytes(buf, size);
  decrypt(buf, size);
}

// wait for a packet to arrive and then exit
void awaitPacket()
{
  while (1)
  {
    if (LoRa.parsePacket())
      break;
  }
}

// advertise this device to other devices and exit when another device says hello
void advertiseConnection()
{
  int lastSendTime = 0;
  while (1)
  {
    // send a hello every 1000 milliseconds
    if (millis() - lastSendTime > 1000)
    {
      sendClear(publicKey, KEY_SIZE);
      lastSendTime = millis();
    }

    // check for any other hellos
    if (LoRa.parsePacket())
      break;
  }
}

void initSession()
{
  // check if there is sufficient entropy stored
  allocateEntropy(KEY_SIZE);

  // generate public/private key pair
  generateKeys();
  displayText("Generated keys!");
  delay(1000);

  // await advertisement from sender
  displayText("Connecting...");
  awaitPacket();

  // recieve foreign public key from sender
  recieveClear(f_publicKey, KEY_SIZE);

  // reply with our public key for generating our shared secret
  sendClear(publicKey, KEY_SIZE);
  displayText("Connection established!");

  // generate secret shared key for encryption
  generateSecret();
  chacha.setKey(sharedKey, KEY_SIZE);

  // recieve IV/Nonce from sender to be used in our ChaCha20
  awaitPacket();
  recieveClear(IV, IV_SIZE);
  chacha.setIV(IV, IV_SIZE);
  logHex("IV: ", IV, IV_SIZE);
}

void allocateEntropy(size_t size)
{
  // check for sufficient entropy
  if (RNG.available(size))
    return;

  // if we need more, generate some from broadband noise
  uint8_t noise[1];
  for (size_t i = 0; i < size; i++)
  {
    noise[0] = LoRa.random();
    RNG.stir(noise, sizeof(noise), sizeof(noise) * 8);
  }

  // check that we have enough, otherwise halt
  if (RNG.available(size))
    return;
  displayText("Insufficient entropy\nExiting...");
  while (1)
    ;
}