/**
 * Hospital station
 * 
 * this station listens for incoming messages from the drone
 */

#define ESP32 1
#define DEBUG 1

#include <heltec.h>

#include "utils.h"

#define BAND 915E6
#define PACKET_SIZE 5
#define MAX_ORDERS 5
#define PACKET_WAIT_TIME 5000
#define INTER_HELLO_TIME 10000

namespace ECE496 {
class Hospital {
    uint8_t orders_buf[MAX_ORDERS][PACKET_SIZE] = {{0}};
    uint8_t total_mem = MAX_ORDERS * PACKET_SIZE;
    uint8_t num_order = 0;

   public:
    int addOrder(uint8_t *order) {
        if (num_order < MAX_ORDERS) {
            memcpy(orders_buf[num_order], order, PACKET_SIZE);
            num_order++;
            return 1;
        } else
            return 0;
    };

    int clearOrders() {
        memset(orders_buf, 0x00, total_mem);
        return 1;
    }

    enum State {
        WAIT = 0,
        EXCHANGE,
        IV,
        RECEIVE,
        CLOSE,
        ERROR,
    };

    static void printState(State state) {
        switch (state) {
            case WAIT:
                ECE496::Utils::displayTextAndScroll("WAIT");
                break;
            case EXCHANGE:
                ECE496::Utils::displayTextAndScroll("EXCHANGE");
                break;
            case IV:
                ECE496::Utils::displayTextAndScroll("IV");
                break;
            case RECEIVE:
                ECE496::Utils::displayTextAndScroll("RECEIVE");
                break;
            case CLOSE:
                ECE496::Utils::displayTextAndScroll("CLOSE");
                break;
            case ERROR:
            default:
                ECE496::Utils::displayTextAndScroll("ERROR");
                break;
        }
    }
};
}  // namespace ECE496

uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];
uint8_t orders_to_receive = 0;
uint8_t i = 0;

ECE496::Hospital::State State = ECE496::Hospital::WAIT;
ECE496::Hospital::State NextState;

ECE496::Hospital *Hospital = new ECE496::Hospital();

void setup() {
    Heltec.begin(true, true, true, true, BAND);
    delay(2000);

    ECE496::Utils::displayTextAndScroll("I am a hospital station.");
}

void loop() {
    switch (State) {
        // send out a HELLO to the drone
        case ECE496::Hospital::WAIT: {
            // wait some time to not clog channel
            delay(INTER_HELLO_TIME);

            // send hello
            ECE496::Utils::buildPacket(s_packet_buf,ECE496::Utils::HELLO,PACKET_SIZE,NULL);
            ECE496::Utils::sendUnencryptedPacket(s_packet_buf,PACKET_SIZE);

            // await a response from a drone
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf,PACKET_SIZE);
                if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE 
                    && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK) {
                    // Continue to encryption
                    NextState = ECE496::Hospital::EXCHANGE;
                }
                else {
                    NextState = ECE496::Hospital::WAIT;
                }
            }
            else {
                NextState = ECE496::Hospital::WAIT;
            }
            break;
        }

        // Exchange public keys with the drone
        case ECE496::Hospital::EXCHANGE: {
            ECE496::Utils::generateKeys();
            ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::publicKey, KEY_SIZE);
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::f_publicKey, KEY_SIZE);
                NextState = ECE496::Hospital::IV;
            } else {
                NextState = ECE496::Hospital::WAIT;
            }
            break;
        }

        // Send IV to drone for encryption
        case ECE496::Hospital::IV: {
            ECE496::Utils::generateIV();
            ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::IV, IV_SIZE);
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                if (ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK && ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE) {
                    // encrypt
                    ECE496::Utils::generateSecret();
                    ECE496::Utils::chacha.setKey(ECE496::Utils::sharedKey, KEY_SIZE);
                    ECE496::Utils::chacha.setIV(ECE496::Utils::IV, IV_SIZE);

                    NextState = ECE496::Hospital::RECEIVE;
                } else {
                    NextState = ECE496::Hospital::WAIT;
                }
            } else {
                NextState = ECE496::Hospital::WAIT;
            }
            break;
        }


        // Receive orders from drone
        case ECE496::Hospital::RECEIVE: {
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receivePacket(r_packet_buf, PACKET_SIZE);

                // make sure packet is from a drone station
                if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE 
                    && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::PAYLOAD 
                    && Hospital->addOrder(r_packet_buf)) {

                    // save order
                    ECE496::Utils::displayTextAndScroll("Got PAYLOAD from drone.");
                    ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::ACK,PACKET_SIZE,NULL);
                    ECE496::Utils::sendPacket(s_packet_buf,PACKET_SIZE);
                    NextState = ECE496::Hospital::RECEIVE;
                } else {
                    Serial.print("Received ill-formed packet.");
                    NextState = ECE496::Hospital::CLOSE;
                }
            } else {
                NextState = ECE496::Hospital::CLOSE;
            }
            break;
        }

        // Close session and clear all crypto-sensitive information from this device
        case ECE496::Hospital::CLOSE: {
            ECE496::Utils::closeSession();
            NextState = ECE496::Hospital::WAIT;
            break;
        }

        // Case for any errors
        case ECE496::Hospital::ERROR:
        default: {
            Serial.println("This shouldn't happen.");
            ECE496::Utils::closeSession();
            while (1)
                ;
            break;
        }
    }

    // move to the next state
    if (DEBUG) {
        delay(100);
        if (State != NextState) {
            ECE496::Hospital::printState(NextState);
        }
    }
    State = NextState;
}
