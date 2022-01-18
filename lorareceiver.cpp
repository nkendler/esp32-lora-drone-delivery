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

void recieveHandler(int packetSize);
void updateNoise();
void initSession();
void advertiseConnection();
void awaitPacket();
String recievePacket();
void recieveKey();
void sendKey();
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

  // enable RF subsystem for hardware TRNG
  wifi_init_config_t wic = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wic);

  delay(2000);

  // generate sufficient entropy for our uses
  RNG.begin("LoRa Reciever");
  updateNoise();

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

void recieveHandler(int packetSize)
{
}

void generateKeys()
{
  Curve25519::dh1(publicKey, privateKey);
}

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

void generateSecret()
{
  Curve25519::dh2(f_publicKey, privateKey);
  memcpy(sharedKey, f_publicKey, KEY_SIZE);
  logHex("Shared secret: ", sharedKey, KEY_SIZE);
}

void displayText(String s)
{
  Heltec.DisplayText(s);
  if (DEBUG)
    Serial.print(s + "\n");
}

void sendPacket(String s)
{
  // convert the input String to an array of bytes
  uint8_t message[s.length() + 1];
  s.toCharArray((char *)message, s.length() + 1);
  if (DEBUG)
    Serial.print(s + " is my message.\n");
  logHex("Sending message: ", message, s.length() + 1);

  // send the message to the recipient in ciphertext
  sendCipher(message, s.length() + 1);
  logHex("Sent cipher: ", message, s.length() + 1);
}

void generateIV()
{
  // check if there is enough entropy in the system for IV
  if (!RNG.available(IV_SIZE))
  {
    displayText("Insufficient entropy\nto generate IV\nExiting...");
    while (1)
      ;
  }

  RNG.rand(IV, IV_SIZE);
}

void sendClear(uint8_t *buf, size_t size)
{
  LoRa.beginPacket();
  LoRa.write(buf, size);
  LoRa.endPacket();
}

void sendCipher(uint8_t *buf, size_t size)
{
  encrypt(buf, size);
  LoRa.beginPacket();
  LoRa.write(buf, size);
  LoRa.endPacket();
}

void encrypt(uint8_t *input, size_t size)
{
  chacha.encrypt(input, input, size);
}

void decrypt(uint8_t *input, size_t size)
{
  logHex("input ", input, size);
  chacha.decrypt(input, input, size);
  logHex("output ", input, size);
}

void sendKey()
{
  LoRa.beginPacket();
  LoRa.write(publicKey, KEY_SIZE);
  LoRa.endPacket();
  if (DEBUG)
    logHex("Sent key ", publicKey, KEY_SIZE);
}

void recieveKey()
{
  LoRa.readBytes(f_publicKey, KEY_SIZE);
  logHex("Recieved key ", f_publicKey, KEY_SIZE);
}

String recievePacket()
{
  // recieve ciphertext message and decrypt it to cleartext
  uint8_t coded[LoRa.available()];
  recieveCipher(coded, LoRa.available());

  // return message as a String
  return String((char *)coded);
}

void recieveClear(uint8_t *buf, size_t size)
{
  LoRa.readBytes(buf, size);
  logHex("Recieved clear: ", buf, size);
}

void recieveCipher(uint8_t *buf, size_t size)
{
  LoRa.readBytes(buf, size);
  logHex("Recieved cipher: ", buf, size);
  decrypt(buf, size);
  logHex("^clear: ", buf, size);
}

void awaitPacket()
{
  while (1)
  {
    if (LoRa.parsePacket())
      break;
  }
}

void advertiseConnection()
{
  int lastSendTime = 0;
  while (1)
  {
    if (millis() - lastSendTime > 1000)
    {
      sendKey();
      lastSendTime = millis();
    }
    if (LoRa.parsePacket())
      break;
  }
}

void initSession()
{
  // check if there is sufficient entropy stored
  if (!RNG.available(KEY_SIZE))
  {
    displayText("Insufficient entropy\nto generate key pair\nExiting...");
    while (1)
      ;
  }

  // generate public/private key pair
  generateKeys();
  displayText("Generated keys!\n");
  logHex("Public key: ", publicKey, KEY_SIZE);
  logHex("Private key: ", privateKey, KEY_SIZE);
  delay(1000);

  // await advertisement from sender
  displayText("Connecting...");
  awaitPacket();
  displayText("Detected other device.");

  // recieve foreign public key from sender
  recieveKey();

  // reply with our public key for generating our shared secret
  for (int i = 0; i < 1; i++)
  {
    sendKey();
    delay(100);
  }
  displayText("Connection established!");

  // generate secret shared key for encryption
  generateSecret();
  chacha.setKey(sharedKey, KEY_SIZE);

  awaitPacket();
  recieveClear(IV, IV_SIZE);
  chacha.setIV(IV,IV_SIZE);
  logHex("Recieved IV: ", IV, IV_SIZE);
}

void updateNoise()
{
  esp_fill_random(noise, 32);
  RNG.stir(noise, sizeof(noise), sizeof(noise) * 8);
}