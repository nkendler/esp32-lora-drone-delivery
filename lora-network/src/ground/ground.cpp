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
String packet;
int i;

void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  Serial.begin(115200);
  Serial.setTimeout(1);

  ECE496::Utils::displayText("I am a ground station");
}

void loop()
{
  //ECE496::Utils::displayText("I am a ground station\nWaiting for order from CLI");
  //wait for an order to come through before advertising connection
  while (Serial.available() < 10); //wait for all bytes to arrive before processing

  //packet is received from CLI. its in string form of the int representation of the
  //packet. ie: if the packet was 0x11101 = 29, then packet is '29'
  packet = Serial.readString();
  Serial.print(packet);
  Serial.println();
  
  // advertise existent to potential drone stations
  ECE496::Utils::buildPacket(s_packet_buf, 1, 1, PACKET_SIZE);
  ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);

  // wait for a response
  if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME) == 1)
  {
    int bytes_received =
      ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
    
    // make sure packet is from a drone station
    if (ECE496::Utils::getPacketStationType(r_packet_buf) == 3)
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
