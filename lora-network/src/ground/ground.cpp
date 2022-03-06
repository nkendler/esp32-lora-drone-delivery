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
      ADVERTISE,
      EXCHANGE,
      IV,
      SEND,
      RECEIVE 
    };
  };
}

// packet buffers
uint8_t r_packet_buf[PACKET_SIZE];
uint8_t s_packet_buf[PACKET_SIZE];
uint8_t order[PACKET_SIZE] = {0};

ECE496::Ground::State State = ECE496::Ground::WAIT;
ECE496::Ground::State NextState; 

int i;
bool has_order = false;

void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  Serial.begin(115200);
  Serial.setTimeout(1);

  ECE496::Utils::displayTextAndScroll("I am a ground station.");
}

void loop()
{
  switch (State)
  {
  case ECE496::Ground::WAIT:
    if (Serial.available() == PACKET_SIZE) //wait for all bytes from packet to arrive serially
    {
      /*packet is received from CLI. its in byte array form and the packet is 5 bytes long
      the byte array sends the first 8 bits and continues until the last 8 bits arrive
      so if the order is 10741946436 = 0x0280451844, then the byte array will have
      0x02 0x80 0x45 0x18 0x44 and order_buf will be the same*/
      for (i = PACKET_SIZE - 1; i >= 0; i--) {
        order[i] = Serial.read();
        delay(100);
      }

      ECE496::Utils::displayTextAndScroll("Received an order upload.");
      has_order = true;
    }
    if (has_order) {
      NextState = ECE496::Ground::BUILD;
    }
    else {
      NextState = ECE496::Ground::WAIT;
    }
    break;
  
  case ECE496::Ground::BUILD:
    ECE496::Utils::buildPacket(s_packet_buf, ECE496::Utils::PAYLOAD,
                               PACKET_SIZE, order);
    // assume success for now
    NextState = ECE496::Ground::ADVERTISE;
    break;

  case ECE496::Ground::ADVERTISE:
      {
        uint8_t hello_buffer[PACKET_SIZE];
        ECE496::Utils::buildPacket(hello_buffer, ECE496::Utils::HELLO,PACKET_SIZE, NULL);
        ECE496::Utils::sendUnencryptedPacket(hello_buffer, PACKET_SIZE);
        if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
          uint8_t reply[PACKET_SIZE];
          ECE496::Utils::receiveUnencryptedPacket(reply, PACKET_SIZE);
          if (ECE496::Utils::getPacketType(reply) == ECE496::Utils::ACK && ECE496::Utils::getPacketStationType(reply) == ECE496::Utils::DRONE) {
            NextState = ECE496::Ground::EXCHANGE;
          }
          else {
            NextState = ECE496::Ground::WAIT;
          }
        }
        else {
          NextState = ECE496::Ground::WAIT;
        }
      }
    break;

  case ECE496::Ground::EXCHANGE:
      {
        ECE496::Utils::generateKeys();
        ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::publicKey, KEY_SIZE);
        if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME)) {
          ECE496::Utils::receiveUnencryptedPacket(ECE496::Utils::f_publicKey, KEY_SIZE);
          NextState = ECE496::Ground::IV;
        }
        else {
          NextState = ECE496::Ground::WAIT;
        }
      }
    break;

  case ECE496::Ground::IV:
      {
        ECE496::Utils::generateIV();
        ECE496::Utils::sendUnencryptedPacket(ECE496::Utils::IV, IV_SIZE);
        if (ECE496::Utils::awaitPacketUntil(PACKET_WAIT_TIME))
        {
          ECE496::Utils::receiveUnencryptedPacket(r_packet_buf, PACKET_SIZE);
          if (ECE496::Utils::getPacketType(r_packet_buf) == ECE496::Utils::ACK && ECE496::Utils::getPacketStationType(r_packet_buf) == ECE496::Utils::DRONE)
          {
            NextState = ECE496::Ground::SEND;
          }
          else {
            NextState = ECE496::Ground::WAIT;
          }
        }
        else
        {
          NextState = ECE496::Ground::WAIT;
        }
      }
    break;

  case ECE496::Ground::SEND:
    ECE496::Utils::displayTextAndScroll("Sending packet.");
    ECE496::Utils::sendUnencryptedPacket(s_packet_buf, PACKET_SIZE);
    NextState = ECE496::Ground::RECEIVE;

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
        NextState = ECE496::Ground::CLEAR;
      }
      else
      {
        Serial.print("Received ill-formed packet.");
        NextState = ECE496::Ground::SEND;
      }
    }
    else {
      NextState = ECE496::Ground::SEND;
    }
    break;

  case ECE496::Ground::CLEAR:
    memset(order, 0x00, PACKET_SIZE);
    NextState = ECE496::Ground::WAIT;
    has_order = false;
    break;

  default:
    Serial.println("This shouldn't happen.");
    while(1);
    break;
  }
  State = NextState;
}
