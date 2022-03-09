#ifndef __ECE496_H__
#define __ECE496_H__

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef KEY_SIZE
#define KEY_SIZE 32
#endif

#ifndef IV_SIZE
#define IV_SIZE 8
#endif

#include <Arduino.h>
#include <ChaCha.h>
#include <Curve25519.h>
#include <RNG.h>
#include <oled/SSD1306Wire.h>

namespace ECE496 {
class Utils {
   public:
    enum StationType {
        GROUND = 1,
        HOSPITAL,
        DRONE,
        UNKNOWN
    };

    enum PacketType {
        HELLO = 1,
        ACK,
        PAYLOAD,
        GOODBYE,
        ERROR
    };

    static void begin(char const* id);
    static void logHex(String n, uint8_t* s, size_t size);
    static void displayText(char const* text);
    static void displayTextAndScroll(char const* text);


    static void sendPacket(uint8_t* buf, size_t size);
    static void sendUnencryptedPacket(uint8_t* buf, size_t pakcet_size);
    static size_t receivePacket(uint8_t* buf, size_t packet_size);
    static size_t receiveUnencryptedPacket(uint8_t* buf, size_t packet_size);
    static void awaitPacket();
    static int awaitPacketUntil(unsigned long timeout);

    static void buildPacket(uint8_t* buf, PacketType packet_type, size_t packet_size, uint8_t* payload);
    static Utils::StationType getPacketStationType(uint8_t* buf);
    static Utils::PacketType getPacketType(uint8_t* buf);

    static void generateKeys();
    static void generateIV();
    static void generateSecret();
    static void closeSession();

    static uint8_t publicKey[KEY_SIZE];
    static uint8_t privateKey[KEY_SIZE];
    static uint8_t f_publicKey[KEY_SIZE];
    static uint8_t sharedKey[KEY_SIZE];
    static uint8_t IV[IV_SIZE];
    static ChaCha chacha;

   private:
    static void allocateEntropy(size_t size);
    static void encrypt(uint8_t* input, size_t size);
    static void decrypt(uint8_t* input, size_t size);

    static unsigned int screenLines;
};
}  // namespace ECE496

#endif /*__ECE496_H__*/
