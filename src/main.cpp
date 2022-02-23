/* 
  Base ground station code to receive serial input from computer
*/
#define ESP32 1
#define DEBUG 1

#include "Arduino.h"
#include "heltec.h"
#include "ECE496.h"

#define BAND 915E6

int counter = 0;

void setup()
{
  Heltec.begin(true, true, true, true, BAND);
  Serial.begin(115200);
  Serial.setTimeout(1);
}

void loop()
{
    while (!Serial.available());
    int x = Serial.readString().toInt();
    Serial.print(x + 1);
    ECE496::displayText(String(x+1);
}