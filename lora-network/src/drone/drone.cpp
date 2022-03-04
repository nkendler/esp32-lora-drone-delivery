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
  };
} 

ECE496::Drone *Drone = new ECE496::Drone();

void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  delay(2000);

  ECE496::Utils::displayTextAndScroll("I am a drone station.");
}

void loop()
{
  ECE496::Utils::displayTextAndScroll("Waiting for packets.");
  //Listen for packets
  ECE496::Utils::awaitPacket();

  //Process incoming packets
  int bytes_received = 
    ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
  ECE496::Utils::logHex("r packet: ", r_packet_buf, PACKET_SIZE);
  // first byte should be 0x00 to introduce a hospital station
  //                      0xFF                ground station
  if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::HOSPITAL)
  {
    //communicating with hospital station
    //TODO:
  }
  else if (ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::GROUND &&
    ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::PAYLOAD)
  {
    ECE496::Utils::displayTextAndScroll("Packet detected from ground station.");

    if (Drone->addOrder(r_packet_buf))
    {
      // send an ack
      ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::DRONE, ECE496::Utils::ACK, PACKET_SIZE, NULL);
      ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);

      ECE496::Utils::logHex("Stored packet: ", r_packet_buf, PACKET_SIZE);
    }

  }
  else
  {
    Serial.print("Received ill-formed packet.");
  }
}