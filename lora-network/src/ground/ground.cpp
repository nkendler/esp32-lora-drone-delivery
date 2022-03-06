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

namespace ECE496
{
  class Ground
  {
  public:
    enum State
    {
      CLEAR = 0,
      WAIT,
      BUILD,
      SEND,
      RECEIVE 
    };
  };
}

// packet buffers
uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];
uint8_t order[PACKET_SIZE];
String packet;

ECE496::Ground::State State = ECE496::Ground::WAIT;

int i;

void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  Serial.begin(115200);
  Serial.setTimeout(1);

  ECE496::Utils::displayTextAndScroll("I am a ground station.");
}

void loop()
{
  ECE496::Ground::State nextState; 

  switch (State)
  {
  case ECE496::Ground::WAIT:
    if (Serial.available() == PACKET_SIZE) //wait for all bytes from packet to arrive serially
    {
      /*packet is received from CLI. its in byte array form and the packet is 5 bytes long
      the byte array sends the first 8 bits and continues until the last 8 bits arrive
      so if the order is 10741946436 = 0x0280451844, then the byte array will have
      0x02 0x80 0x45 0x18 0x44 and order_buf will be 0x44 0x18 0x45 0x80 0x02 */
      for (i = PACKET_SIZE - 1; i >= 0; i--) {
        order[i] = Serial.read();
        delay(100);
      }

      ECE496::Utils::displayTextAndScroll("Received an order upload.");
      nextState = ECE496::Ground::BUILD;
    }
    else
    {
      nextState = ECE496::Ground::WAIT;
    }
    break;
  
  case ECE496::Ground::BUILD:
    ECE496::Utils::buildPacket(s_packet_buf,
                               ECE496::Utils::GROUND, ECE496::Utils::PAYLOAD,
                               PACKET_SIZE, order);
    // assume success for now
    nextState = ECE496::Ground::SEND;
    break;

  case ECE496::Ground::SEND:
    ECE496::Utils::displayTextAndScroll("Sending packet.");
    ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
    nextState = ECE496::Ground::RECEIVE;

  case ECE496::Ground::RECEIVE:
    // wait for a response
    if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME))
    {
      ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
      
      // make sure packet is from a drone station
      if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE &&
        ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK)
      {
        // found a drone station
        ECE496::Utils::displayTextAndScroll("Got ACK from drone.");
        nextState = ECE496::Ground::CLEAR;
      }
      else
      {
        Serial.print("Received ill-formed packet.");
        nextState = ECE496::Ground::SEND;
      }
    }
    else {
      nextState = ECE496::Ground::SEND;
    }
    break;

  case ECE496::Ground::CLEAR:
    memset(order, 0x00, PACKET_SIZE);
    nextState = ECE496::Ground::WAIT;
    break;

  default:
    Serial.println("This shouldn't happen.");
    while(1);
    break;
  }
  State = nextState;
}
