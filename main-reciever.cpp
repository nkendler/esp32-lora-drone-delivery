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


void handleIndex() {
  Serial.println("got request");

  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.printf("got request: %s\n", body);
    oled.clearDisplay();
    oled.setTextSize(1);          // text size
    oled.setTextColor(WHITE);     // text color
    oled.setCursor(0, 0);        // position to display
    oled.println(body); // text to display
    oled.display();               // show on OLED
  } else {
    server.send(200, "text/plain", "no body");
  }

  server.send(200, "text/plain", "printed to oled");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(74880);

  // initialize OLED display with address 0x3C for 128x64
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  Serial.println("setting AP...");
  WiFi.softAP(ssid);

  WiFi.softAPConfig(local_ip, gateway, netmask);
  Serial.print("AP IP address: ");
  Serial.println(local_ip);


  //MDNS.begin("esp8266");

  server.on("/", handleIndex);
  server.onNotFound(handleIndex);
  server.begin();
  //MDNS.addService("http", "tcp", 80);

  delay(2000);         // wait for initializing
  oled.clearDisplay(); // clear display

  oled.setTextSize(1);          // text size
  oled.setTextColor(WHITE);     // text color
  oled.setCursor(0, 0);        // position to display
  oled.println("Fuck You!"); // text to display
  oled.display();               // show on OLED
}

void loop() {
  server.handleClient();
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off
  delay(1000);
}

