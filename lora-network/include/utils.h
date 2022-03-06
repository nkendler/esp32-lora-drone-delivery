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

#include <ChaCha.h>
#include <RNG.h>
#include <Curve25519.h>
#include <Arduino.h>
#include <oled/SSD1306Wire.h>

namespace ECE496 
{
    class Utils
    {
    public:
        static void advertiseConnection();
        static void awaitPacket();
        static int awaitPacketUntil(unsigned long timeout);
        static void receivePacket(char *buf);
        static void sendPacket(char const *s, int size);
        static void sendUnencryptedPacket(uint8_t *buf, int pakcet_size);
        static void displayText(char const *text);
        static void displayTextAndScroll(char const *text);
        static void logHex(String n, uint8_t *s, size_t size);
        static void begin(char const *id);
        static void initSession(bool sender);
        static void closeSession();
        static bool isSender();
        static bool isReceiver();
        static int  receiveUnencryptedPacket(uint8_t *buf, int packet_size);
        static void buildPacket(uint8_t *buf, int packet_type, int packet_size, uint8_t *payload);

        enum StationType
        {
            GROUND = 1,
            HOSPITAL,
            DRONE,
            UNKNOWN
        };

        enum PacketType
        {
            HELLO = 1,
            ACK,
            PAYLOAD,
            ERROR
        };

        static Utils::StationType getPacketStationType(uint8_t *buf);
        static Utils::PacketType getPacketType(uint8_t *buf);

    private:
        static void displayText(String s);
        static void allocateEntropy(size_t size);
        static void generateKeys();
        static void generateIV();
        static void generateSecret();
        static void sendClear(uint8_t *buf, size_t size);
        static void receiveClear(uint8_t *buf, size_t size);
        static void sendCipher(uint8_t *buf, size_t size);
        static void receiveCipher(uint8_t *buf, size_t size);
        static void encrypt(uint8_t *input, size_t size);
        static void decrypt(uint8_t *input, size_t size);

        static bool sender;
        static unsigned int screenLines;
        static uint8_t publicKey[KEY_SIZE];
        static uint8_t privateKey[KEY_SIZE];
        static uint8_t f_publicKey[KEY_SIZE];
        static uint8_t sharedKey[KEY_SIZE];
        static uint8_t IV[IV_SIZE];
        static ChaCha chacha;
    };
}

#endif /*__ECE496_H__*/
