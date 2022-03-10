/**
 * Drone station
 * 
 * this station listens for incoming transmissions
 */

#define ESP32 1

#include <heltec.h>

#include "utils.h"

#define BAND 915E6

#define PACKET_SIZE 5
#define PACKET_WAIT_TIME 5000
#define MAX_ORDERS 5

// packet buffers
uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];

namespace ECE496 {
class Drone {
    uint8_t orders_buf[MAX_ORDERS][PACKET_SIZE] = {{0}};
    int num_orders = 0;

    public:
    int addOrder(uint8_t* order) {
        if (num_orders < MAX_ORDERS) {
            memcpy(orders_buf[num_orders], order, PACKET_SIZE);
            orders_buf[num_orders][0] & 0x0F;  // clear station and packet type info
            num_orders++;
            return 1;
        } else {
            return 0;
        }
    };
    int getOrder(uint8_t* order) {
        if (num_orders > 0) {
            memcpy(order, orders_buf[num_orders], PACKET_SIZE);
            return 1;
        }
        else {
            return 0;
        }
    };
    int popOrder() {
        if (num_orders > 0) {
            memset(orders_buf[num_orders], 0, PACKET_SIZE);
            num_orders--;
            return 1;
        }
        else {
            return 0;
        }
    }
    bool hasSpace() {
        return num_orders < MAX_ORDERS;
    }
    bool hasOrder() {
        return num_orders > 0;
    }
    enum State {
        WAIT = 0,
        VERIFY,
        CLEAR,
        UPLOAD,
        STORE,
        RESPOND,
        GROUND_EXCHANGE,
        GROUND_IV,
        HOSPITAL_EXCHANGE,
        HOSPITAL_IV,
        READY,
        ERROR,
        CLOSE
    };
    static void printState(State state) {
        switch (state) {
            case CLEAR:
                ECE496::Utils::displayTextAndScroll("CLEAR");
                break;
            case WAIT:
                ECE496::Utils::displayTextAndScroll("WAIT");
                break;
            case VERIFY:
                ECE496::Utils::displayTextAndScroll("VERIFY");
                break;
            case UPLOAD:
                ECE496::Utils::displayTextAndScroll("UPLOAD");
                break;
            case GROUND_EXCHANGE:
                ECE496::Utils::displayTextAndScroll("GROUND_EXCHANGE");
                break;
            case GROUND_IV:
                ECE496::Utils::displayTextAndScroll("GROUND_IV");
                break;
            case RESPOND:
                ECE496::Utils::displayTextAndScroll("RESPOND");
                break;
            case READY:
                ECE496::Utils::displayTextAndScroll("READY");
                break;
            case CLOSE:
                ECE496::Utils::displayTextAndScroll("CLOSE");
                break;
            case STORE:
                ECE496::Utils::displayTextAndScroll("STORE");
                break;
            case ERROR:
            default:
                ECE496::Utils::displayTextAndScroll("ERROR");
                break;
        }
    }
};
}  // namespace ECE496

ECE496::Drone* Drone = new ECE496::Drone();

ECE496::Drone::State State = ECE496::Drone::WAIT;
ECE496::Drone::State NextState;

void setup() {
    Heltec.begin(true, true, true, true, BAND);
    delay(2000);
    ECE496::Utils::begin("Drone Station");
    ECE496::Utils::displayTextAndScroll("I am a drone station.");
    if (DEBUG) {
        ECE496::Drone::printState(State);
    }
}

void loop() {
    switch (State) {
        // wait to receive a message from a foreign source
        case ECE496::Drone::WAIT: {
            // Listen for packets, if we get one, head to RECEIVE
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                NextState = ECE496::Drone::VERIFY;
            } else {
                NextState = ECE496::Drone::WAIT;
            }
            break;
        }

        // Determine the packet's purpose
        case ECE496::Drone::VERIFY: {
            if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK) {
                ECE496::Utils::displayTextAndScroll("ACK detected from hospital");
                NextState = ECE496::Drone::UPLOAD;

            } else if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::HELLO) {
                ECE496::Utils::displayTextAndScroll("HELLO detected from hospital");
                
                // check if we have an order to upload
                if (Drone->hasOrder()) {
                    ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::ACK, PACKET_SIZE, NULL);
                    ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
                    NextState = ECE496::Drone::HOSPITAL_EXCHANGE;
                } else {  // otherwise ignore the message
                    ECE496::Utils::displayTextAndScroll("No order to deliver.");
                    NextState = ECE496::Drone::CLOSE;
                }
                
            } else if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::GROUND && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::PAYLOAD) {
                ECE496::Utils::displayTextAndScroll("Packet detected from ground station.");
                NextState = ECE496::Drone::STORE;

            } else if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::GROUND && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::HELLO) {
                ECE496::Utils::displayTextAndScroll("Hello detected from ground station.");

                // check if we have enough space to pickup an order
                if (Drone->hasSpace()) {
                    ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::ACK, PACKET_SIZE, NULL);
                    ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
                    NextState = ECE496::Drone::GROUND_EXCHANGE;
                } else {  // otherwise ignore the message
                    ECE496::Utils::displayTextAndScroll("Insufficient space.");
                    NextState = ECE496::Drone::CLOSE;
                }

            } else {  //unrecognized packet
                ECE496::Utils::displayTextAndScroll("Received ill-formed packet.");
                NextState = ECE496::Drone::CLOSE;
            }
            break;
        }

        // Exchange public keys with the ground station
        case ECE496::Drone::GROUND_EXCHANGE: {
            ECE496::Utils::generateKeys();
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::f_publicKey, KEY_SIZE);
                ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::publicKey, KEY_SIZE);
                NextState = ECE496::Drone::GROUND_IV;
            } else {
                NextState = ECE496::Drone::WAIT;
            }
            break;
        }

        // Receive the IV for encryption from the ground
        case ECE496::Drone::GROUND_IV: {
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::IV, IV_SIZE);
                ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::ACK, PACKET_SIZE, NULL);
                ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);

                // encrypt
                ECE496::Utils::generateSecret();
                ECE496::Utils::chacha.setKey(ECE496::Utils::sharedKey, KEY_SIZE);
                ECE496::Utils::chacha.setIV(ECE496::Utils::IV, IV_SIZE);
                NextState = ECE496::Drone::READY;
            } else {
                NextState = ECE496::Drone::WAIT;
            }

            break;
        }

        // Exchange public keys with the hospital station
        case ECE496::Drone::HOSPITAL_EXCHANGE: {
            ECE496::Utils::generateKeys();
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::f_publicKey, KEY_SIZE);
                ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::publicKey, KEY_SIZE);
                NextState = ECE496::Drone::HOSPITAL_IV;
            } else {
                NextState = ECE496::Drone::WAIT;
            }
            break;
        }
                                        
        // Receive the IV for encryption from the hospital
        case ECE496::Drone::HOSPITAL_IV: {
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::IV, IV_SIZE);
                ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::ACK, PACKET_SIZE, NULL);
                ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);

                // encrypt
                ECE496::Utils::generateSecret();
                ECE496::Utils::chacha.setKey(ECE496::Utils::sharedKey, KEY_SIZE);
                ECE496::Utils::chacha.setIV(ECE496::Utils::IV, IV_SIZE);
                NextState = ECE496::Drone::READY;
            } else {
                NextState = ECE496::Drone::WAIT;
            }

            break;
        }

        // Sends an encypted payload packet to the hospital station
        case ECE496::Drone::UPLOAD: {
            if (Drone->getOrder(s_packet_buf)) {
                ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::PAYLOAD, PACKET_SIZE, s_packet_buf);
                ECE496::Utils::sendPacket(s_packet_buf, PACKET_SIZE);

                // wait for ACK from hospital
                if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                    ECE496::Utils::receivePacket(r_packet_buf, PACKET_SIZE);

                    if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK) {
                        Drone->popOrder(); // remove order if it arrived ok
                        NextState = ECE496::Drone::UPLOAD; // try to send another payload
                    }
                    else {
                        NextState = ECE496::Drone::CLOSE;
                    }
                } else {
                    NextState = ECE496::Drone::CLOSE;
                }
            }
            else {
                // exit secure channel if we have no more orders to upload
                NextState = ECE496::Drone::CLOSE;
            }
            break;
        }

        // Ready to receive a packet over a secure connection
        case ECE496::Drone::READY: {
            // Listen for packets, if we get one, head to VERIFY
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receivePacket(r_packet_buf, PACKET_SIZE);
                NextState = ECE496::Drone::VERIFY;
            } else {
                NextState = ECE496::Drone::CLOSE;
            }
            break;
        }

        // Store the order to memory
        case ECE496::Drone::STORE: {
            if (Drone->addOrder(r_packet_buf)) {
                ECE496::Utils::logHex("Stored packet: ", r_packet_buf, PACKET_SIZE);
                NextState = ECE496::Drone::RESPOND;
            } else {
                ECE496::Utils::logHex("Storage full, ignored packet: ", r_packet_buf, PACKET_SIZE);
                NextState = ECE496::Drone::CLOSE;
            }
            break;
        }

        // Respond with an ACK
        case ECE496::Drone::RESPOND: {
            ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::ACK, PACKET_SIZE, NULL);
            ECE496::Utils::sendPacket(s_packet_buf, PACKET_SIZE);
            NextState = ECE496::Drone::CLOSE;
            break;
        }

        // need to add a state after communcations close in a session to call closeSession()
        case ECE496::Drone::CLOSE: {
            ECE496::Utils::closeSession();
            NextState = ECE496::Drone::WAIT;
            break;
        }

        // Case for any errors
        case ECE496::Drone::ERROR:
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
            ECE496::Drone::printState(NextState);
        }
    }
    State = NextState;
}