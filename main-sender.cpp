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
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

// workaround
#include <Adafruit_I2CDevice.h>


#define SCREEN_WIDTH 128 // OLED display width,  in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// declare an SSD1306 display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char *ssid = "a-esp-12f";

WiFiClient wc = WiFiClient();
IPAddress server(192,168,4,1);
int port = 80;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(74880);

  // initialize OLED display with address 0x3C for 128x64
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  Serial.println("Connecting to ssid: " + String(ssid));
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected successfully to ssid: " + String(ssid));
  Serial.println("The IP Address of this module is "+WiFi.localIP().toString());

  Serial.println("Connecting to server "+server.toString()+" on port "+String(port));
  if (wc.connect(server,port)) {
    Serial.println("Connected!");
    delay(2000);         // wait for initializing
    oled.clearDisplay(); // clear display
    oled.setTextSize(1);          // text size
    oled.setTextColor(WHITE);     // text color
    oled.setCursor(0, 0);        // position to display
    oled.println("I am a sender!"); // text to display
    oled.display();               // show on OLED
  }
  else {
    Serial.println("Connection failed.");
    delay(2000);         // wait for initializing
    oled.clearDisplay(); // clear display

    oled.setTextSize(1);          // text size
    oled.setTextColor(WHITE);     // text color
    oled.setCursor(0, 0);        // position to display
    oled.println("Connection failed."); // text to display
    oled.display();               // show o
  }
}

void loop() {
  delay(5000);
  for (int i=0;i<100;i++) {
    HTTPClient http;
    http.begin(wc,server.toString(),80,"/",false);
    http.addHeader("Content-Type","text/plain");
    
    String message = "Message number "+String(i);
    int http_code = http.POST(message);
    String response = http.getString();
    Serial.println("Message = "+message+", http code = "+String(http_code)+", response = "+response);
    oled.clearDisplay(); // clear display

    oled.setTextSize(1);          // text size
    oled.setTextColor(WHITE);     // text color
    oled.setCursor(0, 0);        // position to display
    oled.println("I am a sender! ("+String(i)+")"); // text to display
    oled.println("Response is "+String(http_code)+"!"); // text to display
    oled.display();               // show o
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off
    delay(5000);
    http.end();
    yield();
  }
  
}

