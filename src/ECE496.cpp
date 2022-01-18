#include "ECE496.h"
#include <Curve25519.h>
#include <RNG.h>
#include <ChaCha.h>
#include "heltec.h"

ChaCha ECE496::chacha;

uint8_t ECE496::publicKey[KEY_SIZE];
uint8_t ECE496::privateKey[KEY_SIZE];
uint8_t ECE496::f_publicKey[KEY_SIZE];
uint8_t ECE496::sharedKey[KEY_SIZE];
uint8_t ECE496::IV[IV_SIZE];

void ECE496::begin(const char* id) {
    chacha = ChaCha();
    RNG.begin(id);
}

// generate public-private key pair using ECDH
void ECE496::generateKeys()
{
    allocateEntropy(KEY_SIZE);
    Curve25519::dh1(publicKey, privateKey);
    logHex("Public key: ", publicKey, KEY_SIZE);
    logHex("Private key: ", privateKey, KEY_SIZE);
}

// log a buffer of bytes to the Serial
void ECE496::logHex(String n, uint8_t *s, size_t size)
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
void ECE496::generateSecret()
{
    Curve25519::dh2(f_publicKey, privateKey);
    memcpy(sharedKey, f_publicKey, KEY_SIZE);
    logHex("Shared secret: ", sharedKey, KEY_SIZE);
}

// display text on the OLED display, and log it to Serial if we're in DEBUG mode
void ECE496::displayText(String s)
{
    Heltec.DisplayText(s);
    if (DEBUG)
        Serial.print(s + "\n");
}

// send an encrypted packet
void ECE496::sendPacket(String s)
{
    // convert the input String to an array of bytes
    uint8_t message[s.length() + 1];
    s.toCharArray((char *)message, s.length() + 1);

    // send the message to the recipient in ciphertext
    sendCipher(message, s.length() + 1);
}

// randomly generate IV/nonce for use in ChaCha20
void ECE496::generateIV()
{
    // check if there is enough entropy in the system for IV
    allocateEntropy(IV_SIZE);

    // randomly generate IV
    RNG.rand(IV, IV_SIZE);
    logHex("IV: ", IV, IV_SIZE);
}

// send a cleartext message
void ECE496::sendClear(uint8_t *buf, size_t size)
{
    LoRa.beginPacket();
    LoRa.write(buf, size);
    LoRa.endPacket();
}

// encrypt a cleartext message into ciphertext and then send it
void ECE496::sendCipher(uint8_t *buf, size_t size)
{
    encrypt(buf, size);
    LoRa.beginPacket();
    LoRa.write(buf, size);
    LoRa.endPacket();
}

// encrypt a buffer
void ECE496::encrypt(uint8_t *input, size_t size)
{
    chacha.encrypt(input, input, size);
}

// decrypt a buffer
void ECE496::decrypt(uint8_t *input, size_t size)
{
    chacha.decrypt(input, input, size);
}

// recieve an encrypted packet
String ECE496::recievePacket()
{
    // recieve ciphertext message and decrypt it to cleartext
    uint8_t coded[LoRa.available()];
    recieveCipher(coded, LoRa.available());

    // return message as a String
    return String((char *)coded);
}

// read cleartext message
void ECE496::recieveClear(uint8_t *buf, size_t size)
{
    LoRa.readBytes(buf, size);
    logHex("Recieved clear: ", buf, size);
}

// read ciphertext message and decrypt it into cleartext
void ECE496::recieveCipher(uint8_t *buf, size_t size)
{
    LoRa.readBytes(buf, size);
    decrypt(buf, size);
}

// wait for a packet to arrive and then exit
void ECE496::awaitPacket()
{
    while (1)
    {
        if (LoRa.parsePacket())
            break;
    }
}

// advertise this device to other devices and exit when another device says hello
void ECE496::advertiseConnection()
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

void ECE496::allocateEntropy(size_t size)
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