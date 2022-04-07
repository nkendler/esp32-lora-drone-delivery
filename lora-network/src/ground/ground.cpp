/**
 * Ground station
 * 
 * this station advertises itself to potential drone stations
 */

#define ESP32 1
#define DEBUG 1

#include <heltec.h>

#include "utils.h"

#define BAND 915E6

#define PACKET_SIZE 5
#define PACKET_WAIT_TIME 5000

namespace ECE496 {
class Ground {
    public:
    enum State {
        CLEAR = 0,
        WAIT,
        BUILD,
        ADVERTISE,
        EXCHANGE,
        IV,
        SEND,
        RECEIVE,
        CLOSE,
        ERROR
    };
    static void printState(State state) {
        switch (state) {
            case CLEAR:
                ECE496::Utils::displayTextAndScroll("CLEAR");
                break;
            case WAIT:
                ECE496::Utils::displayTextAndScroll("WAIT");
                break;
            case BUILD:
                ECE496::Utils::displayTextAndScroll("BUILD");
                break;
            case ADVERTISE:
                ECE496::Utils::displayTextAndScroll("ADVERTISE");
                break;
            case EXCHANGE:
                ECE496::Utils::displayTextAndScroll("EXCHANGE");
                break;
            case IV:
                ECE496::Utils::displayTextAndScroll("IV");
                break;
            case SEND:
                ECE496::Utils::displayTextAndScroll("SEND");
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

// packet buffers
uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];
uint8_t order[PACKET_SIZE] = {0};

ECE496::Ground::State State = ECE496::Ground::WAIT;
ECE496::Ground::State NextState;

int i;
int error = 0;
bool has_order = false;

void setup() {
    Heltec.begin(true, true, true, true, BAND);
    Serial.begin(115200);
    Serial.setTimeout(1);
    ECE496::Utils::begin("Ground Station");
    ECE496::Utils::displayTextAndScroll("I am a ground station.");
    if (DEBUG) {
        ECE496::Ground::printState(State);
    }
}

void loop() {
    switch (State) {

        // Wait for an order to be received over CLI
        case ECE496::Ground::WAIT: {
            if (Serial.available() == PACKET_SIZE)  //wait for all bytes from packet to arrive serially
            {
                /* packet is received from CLI. its in byte array form and the packet is 5 bytes long
      the byte array sends the first 8 bits and continues until the last 8 bits arrive
      so if the order is 0x0280451844, then the byte array will have
      0x02 0x80 0x45 0x18 0x44 and order will be the same */
                for (i = PACKET_SIZE - 1; i >= 0; i--) {
                    order[i] = Serial.read();
                    delay(100);
                }

                ECE496::Utils::displayTextAndScroll("Received an order upload.");
                //send a 0 to ground GUI to let them know to wait
                Serial.print("0");
                Serial.print("\n");
                has_order = true;
            }
            if (has_order) {
                NextState = ECE496::Ground::BUILD;
            } else {
                NextState = ECE496::Ground::WAIT;
            }
            break;
        }

        // Build packet from CLI input
        case ECE496::Ground::BUILD: {
            ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::PAYLOAD, PACKET_SIZE, order);
            // assume success for now
            NextState = ECE496::Ground::ADVERTISE;
            break;
        }

        // Advertise a connection to drones by saying HELLO
        case ECE496::Ground::ADVERTISE: {
            uint8_t hello_buffer[PACKET_SIZE];
            ECE496::Utils::buildPacket(hello_buffer, ECE496::Utils::HELLO, PACKET_SIZE, NULL);
            ECE496::Utils::sendUnencryptedPacket(hello_buffer, PACKET_SIZE);
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                if (ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK && ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE) {
                    NextState = ECE496::Ground::EXCHANGE;
                } else {
                    NextState = ECE496::Ground::WAIT;
                }
            } else {
                NextState = ECE496::Ground::WAIT;
            }
            break;
        }

        // Exchange public keys with the drone
        case ECE496::Ground::EXCHANGE: {
            ECE496::Utils::generateKeys();
            ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::publicKey, KEY_SIZE);
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::f_publicKey, KEY_SIZE);
                NextState = ECE496::Ground::IV;
            } else {
                NextState = ECE496::Ground::WAIT;
            }
            break;
        }

        // Send IV to drone for encryption
        case ECE496::Ground::IV: {
            ECE496::Utils::generateIV();
            ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::IV, IV_SIZE);
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                if (ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK && ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE) {
                    // encrypt
                    ECE496::Utils::generateSecret();
                    ECE496::Utils::chacha.setKey(ECE496::Utils::sharedKey, KEY_SIZE);
                    ECE496::Utils::chacha.setIV(ECE496::Utils::IV, IV_SIZE);

                    NextState = ECE496::Ground::SEND;
                } else {
                    NextState = ECE496::Ground::WAIT;
                }
            } else {
                NextState = ECE496::Ground::WAIT;
            }
            break;
        }

        // Send order to drone as a payload
        case ECE496::Ground::SEND: {
            ECE496::Utils::displayTextAndScroll("Sending packet.");
            ECE496::Utils::sendPacket(s_packet_buf, PACKET_SIZE);
            NextState = ECE496::Ground::RECEIVE;
            break;
        }

        // Receive and confirm ACK from drone
        case ECE496::Ground::RECEIVE: {
            // wait for a response
            if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                
                if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::HELLO) {
                    NextState = ECE496::Ground::RECEIVE;
                }
                else {    
                    ECE496::Utils::decrypt(r_packet_buf,PACKET_SIZE);
                    // make sure packet is from a drone station
                    if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK) {
                        // found a drone station
                        ECE496::Utils::displayTextAndScroll("Got ACK from drone.");
                        NextState = ECE496::Ground::CLEAR;
                    } else {
                        Serial.print("Received ill-formed packet.");
                        NextState = ECE496::Ground::CLOSE;
                    }
                }
            } else {
                NextState = ECE496::Ground::CLOSE;
            }
            break;
        }

        // Close session and clear all crypto-sensitive information from this device
        case ECE496::Ground::CLOSE: {
            ECE496::Utils::closeSession();
            //send a 1 to ground GUI to let them know to close connection and
            // ground station is free to receive a new order
            Serial.print("1");
            Serial.print("\n");
            NextState = ECE496::Ground::WAIT;
            break;
        }

        // Clear packet information after successful upload
        case ECE496::Ground::CLEAR: {
            has_order = false;
            memset(order, 0x00, PACKET_SIZE);
            NextState = ECE496::Ground::CLOSE;
            break;
        }

        // Case for any errors
        case ECE496::Ground::ERROR:
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
            ECE496::Ground::printState(NextState);
        }
    }
    State = NextState;
}
