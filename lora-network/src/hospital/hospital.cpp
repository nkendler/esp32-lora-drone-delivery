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
#define WAIT_TIME_HOSPITAL 10000
#define WAIT_TIME_CONNECT 1000

namespace ECE496
{
  class Hospital
  {
      uint8_t orders_buf[MAX_ORDERS][PACKET_SIZE] = { { NULL } };
      uint8_t total_mem = MAX_ORDERS * PACKET_SIZE;
      uint8_t num_order = 0;
  public:
    int addOrder(uint8_t *order)
    {
      if (num_order < MAX_ORDERS)
      {
        memcpy(orders_buf[num_order], order, PACKET_SIZE);
        num_order++;
        return 1;
      }
      else return 0;
    };

    int clearOrders() {
        memset(orders_buf, 0x00, total_mem);
        return 1;
    }
    enum State
    {
      WAIT = 0,
      CONNECT,
      EXCHANGE,
      RECEIVEORDERNUM,
      RECEIVEORDERS,
      SEND,
      CLEAR 
    };
  };
}

uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];
uint8_t orders_to_receive = 0;
uint8_t i = 0;

ECE496::Hospital::State State = ECE496::Hospital::WAIT;
ECE496::Hospital::State nextState;

ECE496::Hospital *Hospital = new ECE496::Hospital();


void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  delay(2000);

  ECE496::Utils::displayTextAndScroll("I am a hospital station.");
}

void loop()
{

    switch (State)
    {
        case ECE496::Hospital::WAIT:
            delay(WAIT_TIME_HOSPITAL);
            nextState = ECE496::Hospital::CONNECT;
            break;
  
        case ECE496::Hospital::CONNECT:
            //build packet
            ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::HOSPITAL, ECE496::Utils::HELLO, PACKET_SIZE, NULL);
            //send packet
            ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
            //wait to receive something back
            if (ECE496::Utils::awaitPacketUntil(WAIT_TIME_CONNECT)) {
               ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                
                //ensure packet came from drone station
               if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE && ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK) {
                   //received ack, now time to exchange public keys
                   ECE496::Utils::displayTextAndScroll("Got ACK from drone.");
                   nextState = ECE496::Hospital::EXCHANGE;
               }
               else {
                   Serial.print("Received ill-formed packet.");
                   nextState = ECE496::Hospital::WAIT;
               }
            }
            //else, go back to wait
            else {
                nextState = ECE496::Hospital::WAIT;
            }
            break;

        case ECE496::Hospital::EXCHANGE:
            //TO DO: code to exchange and establish secret private key (init session)
            nextState = ECE496::Hospital::RECEIVEORDERNUM;
            break;

        case ECE496::Hospital::RECEIVEORDERNUM:
            //receive encrypted packet
            ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
            // TO DO: unencrypt to find order num
            // TO DO: set orders_to_receive
            nextState = ECE496::Hospital::RECEIVEORDERS;
            break;

        case ECE496::Hospital::RECEIVEORDERS:
            for (i = 0; i < orders_to_receive; i++) {
                //receive order in r_packet_buf
                ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
                Hospital->addOrder(r_packet_buf);
                delay(100);
            }
            nextState = ECE496::Hospital::SEND;
            break;

        case ECE496::Hospital::SEND:
            //add code to connect to cli/database
            nextState = ECE496::Hospital::CLEAR;
            break;

        case ECE496::Hospital::CLEAR:
            orders_to_receive = 0;
            Hospital->clearOrders();
            nextState = ECE496::Hospital::WAIT;
            break;

        default:
            Serial.println("This shouldn't happen.");
            while(1);
            break;
        }
    State = nextState;
}

/*in setup
begin, initsession, 

in loop*/
