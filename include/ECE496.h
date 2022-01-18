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

#include "Arduino.h"
#include <ChaCha.h>
#include <RNG.h>
#include <Curve25519.h>

class ECE496
{
public:
    static void advertiseConnection();
    static void awaitPacket();
    static void awaitPacketUntil();
    static String recievePacket();
    static void sendPacket(String s);
    static void displayText(String s);
    static void logHex(String n, uint8_t *s, size_t size);
    static void begin(const char *id);
    static void initSession(bool sender);
    void ECE496::closeSession();
    static bool isSender();
    static bool isReciever();

private:
    static void allocateEntropy(size_t size);
    static void generateKeys();
    static void generateIV();
    static void generateSecret();
    static void sendClear(uint8_t *buf, size_t size);
    static void recieveClear(uint8_t *buf, size_t size);
    static void sendCipher(uint8_t *buf, size_t size);
    static void recieveCipher(uint8_t *buf, size_t size);
    static void encrypt(uint8_t *input, size_t size);
    static void decrypt(uint8_t *input, size_t size);

    static bool sender;
    static uint8_t publicKey[KEY_SIZE];
    static uint8_t privateKey[KEY_SIZE];
    static uint8_t f_publicKey[KEY_SIZE];
    static uint8_t sharedKey[KEY_SIZE];
    static uint8_t IV[IV_SIZE];
    static ChaCha chacha;

    ECE496() {}
    ~ECE496() {}
};

#endif /*__ECE496_H__*/
