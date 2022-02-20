#include "utils.h"

#include <Curve25519.h>
#include <RNG.h>
#include <ChaCha.h>
#include "heltec.h"
#include <oled/SSD1306Wire.h>

using namespace ECE496;

ChaCha Utils::chacha;

uint8_t Utils::publicKey[KEY_SIZE];
uint8_t Utils::privateKey[KEY_SIZE];
uint8_t Utils::f_publicKey[KEY_SIZE];
uint8_t Utils::sharedKey[KEY_SIZE];
uint8_t Utils::IV[IV_SIZE];


bool Utils::sender;

void Utils::begin(const char *id)
{
    chacha = ChaCha();
    RNG.begin(id);
    sender = false;
}

// generate public-private key pair using ECDH
void Utils::generateKeys()
{
    allocateEntropy(KEY_SIZE);
    Curve25519::dh1(publicKey, privateKey);
    logHex("Public key: ", publicKey, KEY_SIZE);
    logHex("Private key: ", privateKey, KEY_SIZE);
}

// log a buffer of bytes to the Serial
void Utils::logHex(String n, uint8_t *s, size_t size)
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
void Utils::generateSecret()
{
    Curve25519::dh2(f_publicKey, privateKey);
    memcpy(sharedKey, f_publicKey, KEY_SIZE);
    logHex("Shared secret: ", sharedKey, KEY_SIZE);
}

// display text on the OLED display, and log it to Serial if we're in DEBUG mode
void Utils::displayText(String s)
{
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, s);
    Heltec.display->display();
    if (DEBUG)
        Serial.print(s + "\n");
}
void Utils::displayText(char *text)
{
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, text);
    Heltec.display->display();
    delay(300);
}

// send an encrypted packet
void Utils::sendPacket(String s)
{
    // convert the input String to an array of bytes
    uint8_t message[s.length() + 1];
    s.toCharArray((char *)message, s.length() + 1);

    // send the message to the recipient in ciphertext
    sendCipher(message, s.length() + 1);
}

// randomly generate IV/nonce for use in ChaCha20
void Utils::generateIV()
{
    // check if there is enough entropy in the system for IV
    allocateEntropy(IV_SIZE);

    // randomly generate IV
    RNG.rand(IV, IV_SIZE);
    logHex("IV: ", IV, IV_SIZE);
}

// send a cleartext message
void Utils::sendClear(uint8_t *buf, size_t size)
{
    LoRa.beginPacket();
    LoRa.write(buf, size);
    LoRa.endPacket();
}

// encrypt a cleartext message into ciphertext and then send it
void Utils::sendCipher(uint8_t *buf, size_t size)
{
    encrypt(buf, size);
    LoRa.beginPacket();
    LoRa.write(buf, size);
    LoRa.endPacket();
}

// encrypt a buffer
void Utils::encrypt(uint8_t *input, size_t size)
{
    chacha.encrypt(input, input, size);
}

// decrypt a buffer
void Utils::decrypt(uint8_t *input, size_t size)
{
    chacha.decrypt(input, input, size);
}

// recieve an encrypted packet
String Utils::recievePacket()
{
    // recieve ciphertext message and decrypt it to cleartext
    uint8_t coded[LoRa.available()];
    recieveCipher(coded, LoRa.available());

    // return message as a String
    return String((char *)coded);
}

// read cleartext message
void Utils::recieveClear(uint8_t *buf, size_t size)
{
    LoRa.readBytes(buf, size);
    logHex("Recieved clear: ", buf, size);
}

// read ciphertext message and decrypt it into cleartext
void Utils::recieveCipher(uint8_t *buf, size_t size)
{
    LoRa.readBytes(buf, size);
    logHex("Coded: ", buf, size);
    decrypt(buf, size);
    logHex("Uncoded: ", buf, size);
}

// wait for a packet to arrive and then exit
void Utils::awaitPacket()
{
    while (1)
    {
        if (LoRa.parsePacket())
            break;
    }
}

// wait for a packet to arrive and then exit
void Utils::awaitPacketUntil(unsigned long timeout)
{
    unsigned long init = millis();
    while (millis() - init < timeout)
    {
        if (LoRa.parsePacket())
            break;
    }
}

// advertise this device to other devices and exit when another device says hello
void Utils::advertiseConnection()
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

void Utils::allocateEntropy(size_t size)
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

bool Utils::isSender()
{
    return Utils::sender;
}

bool Utils::isReciever()
{
    return !Utils::sender;
}

void Utils::initSession(bool sender)
{
    // determine if we are a sender
    Utils::sender = sender;

    // generate public/private key pair
    generateKeys();
    displayText("Generated keys!");
    delay(1000);

    // Wait for a connection with another device
    displayText("Connecting...");
    if (isSender())
    {
        advertiseConnection();
    }
    else
    {
        awaitPacket();
    }

    // recieve foreign public key from sender
    recieveClear(f_publicKey, KEY_SIZE);

    if (isReciever())
    {
        // reply with our public key for generating our shared secret
        sendClear(publicKey, KEY_SIZE);
    }
    displayText("Connection established!");

    // generate secret shared key for encryption
    generateSecret();
    chacha.setKey(sharedKey, KEY_SIZE);

    if (isSender())
    {
        // generate IV for encryption with ChaCha and send to reciever
        generateIV();
        sendClear(IV, IV_SIZE);
    }
    else
    {
        // recieve IV/Nonce from sender to be used in our ChaCha20
        awaitPacket();
        recieveClear(IV, IV_SIZE);
        logHex("IV: ", IV, IV_SIZE);
    }
    chacha.setIV(IV, IV_SIZE);
}

// destroys all cryptographically-sensitive information from the session
void Utils::closeSession()
{
    sender = false;
    chacha.clear();
    memset(IV, 0, IV_SIZE);
    memset(f_publicKey, 0, KEY_SIZE);
    memset(sharedKey, 0, KEY_SIZE);
    memset(publicKey, 0, KEY_SIZE);
    memset(privateKey, 0, KEY_SIZE);
}
