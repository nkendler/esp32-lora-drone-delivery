/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-oled
 */

#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// workaround
#include <Adafruit_I2CDevice.h>


#define SCREEN_WIDTH 128 // OLED display width,  in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// declare an SSD1306 display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char *ssid = "a-esp-12f";

ESP8266WebServer server(80);
IPAddress local_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress netmask(255,255,255,0);

void init_oled() {
  // initialize OLED display with address 0x3C for 128x64
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  
  // init OLED
  oled.clearDisplay(); 
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
}

void putln(String s) {
  Serial.println(s);
  oled.setCursor(0, 0);
  oled.clearDisplay();
  oled.println(s);
  oled.display();
}

void put(String s) {
  Serial.print(s);
  oled.print(s);
  oled.display();
}

void debug(String s,int n) {
  Serial.println(s+": "+String(n));
}

void debug(String s) {
  Serial.println(s);
}

void handleIndex() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    putln("Got request: \n"+body);
    server.send(200, "text/plain", "Acknowledge "+body);
  } else {
    server.send(400, "text/plain", "Message body empty.");
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(74880);

  init_oled();

  putln("Setting AP...");
  WiFi.softAP(ssid);

  //WiFi.softAPConfig(local_ip, gateway, netmask);
  putln("AP IP address: ");
  put(local_ip.toString()+"\n");

  // handle requests
  server.on("/", handleIndex);
  server.onNotFound(handleIndex);
  server.begin();

  delay(2000);
}

void loop() {
  server.handleClient();
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off
  delay(1000);
}

