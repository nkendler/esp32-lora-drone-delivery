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

#define PACKET_SIZE 1
#define PACKET_WAIT_TIME 5000

// packet buffers
uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];

void setup()
{
  Heltec.begin(true, true, true, true, BAND);

  ECE496::Utils::displayText("I am a ground station");
}

void loop()
{
  // advertise existent to potential drone stations
  s_packet_buf[0] = 0xFF;
  ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);

  // wait for a response
  if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME) == 1)
  {
    int bytes_received =
      ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
    
    // first byte should be 0x0F to indicate drone station response
    if (r_packet_buf[0] == 0x0F)
    {
      // found a drone station
      ECE496::Utils::displayText("I found a drone station");
    }
    else
    {
      Serial.print("Received unrecognized packet");
    }
  }

}