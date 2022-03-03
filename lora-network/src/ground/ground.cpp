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

  // ECE496::Utils::displayTextAndScroll("I am a ground station");
}

void loop()
{
  ECE496::Ground::State nextState; 

  switch (State)
  {
  case ECE496::Ground::WAIT:
    //ECE496::Utils::displayTextAndScroll("wait");
    if (/*Serial.available() >= 10*/true)
    {
      //packet is received from CLI. its in string form of the int representation of the
      //packet. ie: if the packet was 0x11101 = 29, then packet is '29'
      // packet = Serial.readString();
      // Serial.print(packet);
      // Serial.println();
      memset(order, 0xFF, PACKET_SIZE);
      order[0] = 0x0F;
      ECE496::Utils::displayTextAndScroll("got an order");
      nextState = ECE496::Ground::BUILD;
    }
    else
    {
      nextState = ECE496::Ground::WAIT;
    }
    break;
  
  case ECE496::Ground::BUILD:
    //ECE496::Utils::displayTextAndScroll("build");
    ECE496::Utils::buildPacket(s_packet_buf, 1, 3, PACKET_SIZE, order);
    // assume success for now
    nextState = ECE496::Ground::SEND;
    break;

  case ECE496::Ground::SEND:
    ECE496::Utils::displayTextAndScroll("send");
    ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
    nextState = ECE496::Ground::RECEIVE;

  case ECE496::Ground::RECEIVE:
    //ECE496::Utils::displayTextAndScroll("receive");
    // wait for a response
    if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME))
    {
      int bytes_received =
        ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
      
      // make sure packet is from a drone station
      if (ECE496::Utils::getPacketStationType(r_packet_buf) == 3)
      {
        // found a drone station
        ECE496::Utils::displayTextAndScroll("Got ack from drone station");
        nextState = ECE496::Ground::CLEAR;
      }
      else
      {
        Serial.print("Received unrecognized packet");
        nextState = ECE496::Ground::SEND;
      }
    }
    else {
      nextState = ECE496::Ground::SEND;
    }
    // while(1);
    break;

  case ECE496::Ground::CLEAR:
    //ECE496::Utils::displayTextAndScroll("clear");
    memset(order, 0x00, PACKET_SIZE);
    while(1);
    nextState = ECE496::Ground::WAIT;
    break;

  default:
    Serial.println("This shouldn't happen");
    nextState = ECE496::Ground::WAIT;
    break;
  }
  State = nextState;
  //delay(500);
}
