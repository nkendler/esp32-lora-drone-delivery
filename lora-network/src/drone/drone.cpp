/**
 * Drone station
 * 
 * this station listens for incoming transmissions
 */

#define ESP32 1
#define DEBUG 1

#include <heltec.h>
#include "utils.h"

#define BAND 915E6

#define PACKET_SIZE 5
#define PACKET_WAIT_TIME 5000
#define MAX_ORDERS 5

// packet buffers
uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];

namespace ECE496
{
  class Drone
  {
    uint8_t orders_buf[MAX_ORDERS][PACKET_SIZE] = { { NULL } };
    int num_orders = 0;
  public:
    int addOrder(uint8_t *order)
    {
      if (num_orders < MAX_ORDERS)
      {
        memcpy(orders_buf[num_orders], order, PACKET_SIZE);
        num_orders++;
        return 1;
      }
      else return 0;
    };
    enum State
    {
      WAIT = 0,
      RECEIVE,
      VERIFY,
      CLEAR,
      UPLOAD,
      STORE,
      RESPOND
    };
  };
} 

ECE496::Drone *Drone = new ECE496::Drone();

ECE496::Drone::State State = ECE496::Drone::WAIT;
ECE496::Drone::State NextState;

void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  delay(2000);

  ECE496::Utils::displayTextAndScroll("I am a drone station.");
}

void loop()
{
  switch (State)
  {
  case ECE496::Drone::WAIT:
    ECE496::Utils::displayTextAndScroll("Waiting for packets.");
    //Listen for packets
    if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME))
    {
      NextState = ECE496::Drone::RECEIVE;
    }
    else
    {
      NextState = ECE496::Drone::WAIT;
    }
    break;

  case ECE496::Drone::RECEIVE:
    ECE496::Utils::displayTextAndScroll("Got a packet.");
    //Process incoming packets
    ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
    ECE496::Utils::logHex("r packet: ", r_packet_buf, PACKET_SIZE);
    NextState = ECE496::Drone::VERIFY;
    break;

  case ECE496::Drone::VERIFY:
    if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL &&
      ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK)
    {
      ECE496::Utils::displayTextAndScroll("ACK detected from hospital");
      NextState = ECE496::Drone::CLEAR;
    }
    else if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL &&
      ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::HELLO)
    {
      ECE496::Utils::displayTextAndScroll("HELLO detected from hospital");
      NextState = ECE496::Drone::UPLOAD;
    }
    else if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::GROUND &&
      ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::PAYLOAD)
    {
      ECE496::Utils::displayTextAndScroll("Packet detected from ground station.");
      NextState = ECE496::Drone::STORE;
    }
    else //unrecognized packet
    {
      ECE496::Utils::displayTextAndScroll("Received ill-formed packet.");
      NextState = ECE496::Drone::WAIT;
    }
    break;

  case ECE496::Drone::STORE:
    if (Drone->addOrder(r_packet_buf))
    {
      ECE496::Utils::logHex("Stored packet: ", r_packet_buf, PACKET_SIZE);
      NextState = ECE496::Drone::RESPOND;
    }
    else
    {
      NextState = ECE496::Drone::WAIT;
    }
    break;

  case ECE496::Drone::RESPOND:
    // send an ack
    ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::DRONE, ECE496::Utils::ACK, PACKET_SIZE, NULL);
    ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
    break;

  default:
    Serial.println("This shouldn't happen.");
    while(1);
    break;
  }
  State = NextState;
}