#include "utils.h"

#include <Curve25519.h>
#include <RNG.h>
#include <ChaCha.h>
#include "heltec.h"
#include <oled/SSD1306Wire.h>

using namespace ECE496;

ChaCha Utils::chacha;

unsigned int Utils::screenLines = 0;

uint8_t Utils::publicKey[KEY_SIZE];
uint8_t Utils::privateKey[KEY_SIZE];
uint8_t Utils::f_publicKey[KEY_SIZE];
uint8_t Utils::sharedKey[KEY_SIZE];
uint8_t Utils::IV[IV_SIZE];

bool Utils::sender;

void Utils::begin(char const *id)
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
void Utils::displayText(char const *text)
{
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, text);
    Heltec.display->display();
    if (DEBUG)
    {
        Serial.print(text);
        Serial.print("\n");
    }
}

void Utils::displayTextAndScroll(char const *text)
{
    if (screenLines % 6 == 0)
    {
        Heltec.display->clear();
    }
    Heltec.display->drawString(0, (screenLines % 6) * 10, text);
    Heltec.display->display();
    if (DEBUG)
    {
        Serial.print(text);
        Serial.print("\n");
    }
    screenLines++;
}

// send an encrypted packet
void Utils::sendPacket(char const *s, int size)
{
    // send the message to the recipient in ciphertext
    sendCipher((uint8_t *)s, size);
}

void Utils::sendUnencryptedPacket(uint8_t *buf, int packet_size)
{
    LoRa.beginPacket();
    LoRa.write(buf, packet_size);
    LoRa.endPacket();

    logHex("Sent: ", buf, packet_size);
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

// receive an encrypted packet
void Utils::receivePacket(char *buf)
{
    // receive ciphertext message and decrypt it to cleartext
    receiveCipher((uint8_t *)buf, LoRa.available());
}

// reads up to packet_size bytes into the buffer
//returns the actual number of bytes read
int Utils::receiveUnencryptedPacket(uint8_t *buf, int packet_size)
{
    int bytes_available = LoRa.available();
    int bytes_read = packet_size <= bytes_available ? LoRa.readBytes(buf, packet_size) : LoRa.readBytes(buf, bytes_available);

    logHex("received: ", buf, bytes_read);

    return bytes_read;
}

// read cleartext message
void Utils::receiveClear(uint8_t *buf, size_t size)
{
    LoRa.readBytes(buf, size);
    logHex("Received clear: ", buf, size);
}

// read ciphertext message and decrypt it into cleartext
void Utils::receiveCipher(uint8_t *buf, size_t size)
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
int Utils::awaitPacketUntil(unsigned long timeout)
{
    unsigned long init = millis();
    while (millis() - init < timeout)
    {
        if (LoRa.parsePacket())
        {
            return 1;
        }
    }

    return 0;
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

bool Utils::isReceiver()
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

    // receive foreign public key from sender
    receiveClear(f_publicKey, KEY_SIZE);

    if (isReceiver())
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
        // generate IV for encryption with ChaCha and send to receiver
        generateIV();
        sendClear(IV, IV_SIZE);
    }
    else
    {
        // receive IV/Nonce from sender to be used in our ChaCha20
        awaitPacket();
        receiveClear(IV, IV_SIZE);
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

void Utils::buildPacket(uint8_t *buf, int station_type, int packet_type, int packet_size, uint8_t *payload)
{
    // init packet to payload, otherwise fill it with zeroes
    if (payload != NULL)
    {
        memcpy(buf, payload, packet_size);
    }
    else
    {
        memset(buf, 0, packet_size);
    }

    // add station information to front two bits to packet
    switch (station_type)
    {
    case GROUND:
        break;
    case HOSPITAL:
        buf[0] |= 0b01000000;
        break;
    case DRONE:
        buf[0] |= 0b10000000;
        break;
    }

    // add packet type information to third and fourth bits of the packet
    switch (packet_type)
    {
    case HELLO:
        break;
    case ACK:
        buf[0] |= 0b00010000;
        break;
    case PAYLOAD:
        buf[0] |= 0b00100000;
    }
}

Utils::StationType Utils::getPacketStationType(uint8_t *buf)
{
    uint8_t header = buf[0];
    header &= 0b11000000;

    switch (header)
    {
    case 0b00000000:
        return GROUND;
    case 0b01000000:
        return HOSPITAL;
    case 0b10000000:
        return DRONE;
    default:
        return Utils::StationType::ERROR;
    }
}

Utils::PacketType Utils::getPacketType(uint8_t *buf)
{
    uint8_t header = buf[0];
    header &= 0b00110000;

    switch (header)
    {
    case 0b00000000:
        return HELLO;
    case 0b00010000:
        return ACK;
    case 0b00100000:
        return PAYLOAD;
    default:
        return Utils::PacketType::PERROR;
    }
}
