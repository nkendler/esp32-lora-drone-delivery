#include "utils.h"

#include <ChaCha.h>
#include <Curve25519.h>
#include <RNG.h>
#include <oled/SSD1306Wire.h>

#include "heltec.h"

using namespace ECE496;

ChaCha Utils::chacha;

unsigned int Utils::screenLines = 0;

uint8_t Utils::publicKey[KEY_SIZE];
uint8_t Utils::privateKey[KEY_SIZE];
uint8_t Utils::f_publicKey[KEY_SIZE];
uint8_t Utils::sharedKey[KEY_SIZE];
uint8_t Utils::IV[IV_SIZE];

// Calls functions to initialize everything in this class properly
void Utils::begin(char const* id) {
    chacha = ChaCha();
    RNG.begin(id);
}

// Log a buffer of bytes to the Serial
void Utils::logHex(String n, uint8_t* s, size_t size) {
    if (!DEBUG)
        return;
    Serial.print(n);
    for (size_t i = 0; i < size; i++) {
        Serial.printf("%02x", *(s + i));
    }
    Serial.println();
}

// display text on the OLED display, and log it to Serial if we're in DEBUG mode
void Utils::displayText(char const* text) {
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, text);
    Heltec.display->display();
    if (DEBUG) {
        Serial.println(text);
    }
}

// display text on the OLED display and scroll down
void Utils::displayTextAndScroll(char const* text) {
    if (screenLines % 6 == 0) {
        Heltec.display->clear();
    }
    Heltec.display->drawString(0, (screenLines % 6) * 10, text);
    Heltec.display->display();
    if (DEBUG) {
        Serial.print(text);
        Serial.print("\n");
    }
    screenLines++;
}

// send an encrypted packet
void Utils::sendPacket(uint8_t* buf, size_t size) {
    // send the message to the recipient in ciphertext
    logHex("Unencrypted:", buf, size);
    encrypt(buf, size);
    LoRa.beginPacket();
    LoRa.write(buf, size);
    LoRa.endPacket();
    logHex("^ Sent encrypted:", buf, size);
}

// send an unencrypted packet
void Utils::sendUnencryptedPacket(uint8_t* buf, size_t packet_size) {
    LoRa.beginPacket();
    LoRa.write(buf, packet_size);
    LoRa.endPacket();

    logHex("Sent unencrypted:", buf, packet_size);
}

// receive an encrypted packet
// returns the actual number of bytes read
size_t Utils::receivePacket(uint8_t* buf, size_t packet_size) {
    // receive ciphertext message and decrypt it to cleartext
    int bytes_available = LoRa.available();
    size_t bytes_read = packet_size <= bytes_available ? LoRa.readBytes(buf, packet_size) : LoRa.readBytes(buf, bytes_available);
    logHex("Received encrypted: ", buf, bytes_read);
    decrypt(buf, bytes_read);
    logHex("^ Unencrypted: ", buf, bytes_read);

    return bytes_read;
}

// reads up to packet_size bytes into the buffer
// returns the actual number of bytes read
size_t Utils::receiveUnencryptedPacket(uint8_t* buf, size_t packet_size) {
    int bytes_available = LoRa.available();
    size_t bytes_read = packet_size <= bytes_available ? LoRa.readBytes(buf, packet_size) : LoRa.readBytes(buf, bytes_available);

    logHex("Received unencrypted: ", buf, bytes_read);

    return bytes_read;
}

// wait for a packet to arrive
void Utils::awaitPacket() {
    while (1) {
        if (LoRa.parsePacket())
            break;
    }
}

// wait for a packet to arrive and then exit when we reach timeout.
// Return 0 if we timed out,  and 1 if we get a packet
int Utils::awaitPacketUntil(unsigned long timeout) {
    unsigned long init = millis();
    while (millis() - init < timeout) {
        if (LoRa.parsePacket()) {
            return 1;
        }
    }

    return 0;
}

// Constructs a packet formatted with the correct PacketType
// and the station's StationType and fills it with payload
void Utils::buildPacket(uint8_t* buf, Utils::PacketType packet_type, size_t packet_size, uint8_t* payload) {
    // init packet to payload, otherwise fill it with zeroes
    if (payload != NULL) {
        memcpy(buf, payload, packet_size);
    } else {
        memset(buf, 0, packet_size);
    }

    // add station information to front two bits to packet
#ifdef GROUND_STATION
#endif
#ifdef HOSPITAL_STATION
    buf[0] |= 0b01000000;
#endif
#ifdef DRONE_STATION
    buf[0] |= 0b10000000;
#endif

    // add packet type information to third and fourth bits of the packet
    switch (packet_type) {
        case HELLO:
            break;
        case ACK:
            buf[0] |= 0b00010000;
            break;
        case PAYLOAD:
            buf[0] |= 0b00100000;
            break;
        case GOODBYE:
            buf[0] |= 0b00110000;
            break;
    }
}

// Returns the source StationType of a packet
Utils::StationType Utils::getPacketStationType(uint8_t* buf) {
    uint8_t header = buf[0];
    header &= 0b11000000;

    switch (header) {
        case 0b00000000:
            return GROUND;
        case 0b01000000:
            return HOSPITAL;
        case 0b10000000:
            return DRONE;
        default:
            return UNKNOWN;
    }
}

// Returns the PacketType of a packet
Utils::PacketType Utils::getPacketType(uint8_t* buf) {
    uint8_t header = buf[0];
    header &= 0b00110000;

    switch (header) {
        case 0b00000000:
            return HELLO;
        case 0b00010000:
            return ACK;
        case 0b00100000:
            return PAYLOAD;
        case 0b00110000:
            return GOODBYE;
        default:
            return ERROR;
    }
}

// Allocates size bytes of entropy, sourced from radio noise
void Utils::allocateEntropy(size_t size) {
    // check for sufficient entropy
    if (RNG.available(size))
        return;

    // if we need more, generate some from broadband noise
    uint8_t noise[1];
    for (size_t i = 0; i < size; i++) {
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

// Generate a public-private key pair using ECDH
void Utils::generateKeys() {
    allocateEntropy(KEY_SIZE);
    Curve25519::dh1(publicKey, privateKey);
    logHex("Public key: ", publicKey, KEY_SIZE);
    logHex("Private key: ", privateKey, KEY_SIZE);
}

// Generate a shared secret from ECDH
void Utils::generateSecret() {
    Curve25519::dh2(f_publicKey, privateKey);
    memcpy(sharedKey, f_publicKey, KEY_SIZE);
    logHex("Shared secret: ", sharedKey, KEY_SIZE);
}

// randomly generate IV/nonce for use in ChaCha20
void Utils::generateIV() {
    // check if there is enough entropy in the system for IV
    allocateEntropy(IV_SIZE);

    // randomly generate IV
    RNG.rand(IV, IV_SIZE);
    logHex("IV: ", IV, IV_SIZE);
}

// destroys all cryptographically-sensitive information from the session
void Utils::closeSession() {
    chacha.clear();
    memset(IV, 0, IV_SIZE);
    memset(f_publicKey, 0, KEY_SIZE);
    memset(sharedKey, 0, KEY_SIZE);
    memset(publicKey, 0, KEY_SIZE);
    memset(privateKey, 0, KEY_SIZE);
}

// encrypt a buffer
void Utils::encrypt(uint8_t* input, size_t size) {
    chacha.encrypt(input, input, size);
}

// decrypt a buffer
void Utils::decrypt(uint8_t* input, size_t size) {
    chacha.decrypt(input, input, size);
}