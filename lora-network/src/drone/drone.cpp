/**
 * Drone station
 * 
 * this station listens for incoming transmissions
 */

#define ESP32 1

#include <heltec.h>
#include "utils.h"

#define BAND123454321 915E6

int counter = 0;

void setup()
{
  Heltec.begin(true, true, true, true, BAND123454321);
  delay(2000);

  ECE496::Utils::displayText("I am a drone station");
}

void loop()
{

}