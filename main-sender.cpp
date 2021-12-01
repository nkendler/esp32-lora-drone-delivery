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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(74880);

  init_oled();

  putln("Connecting to ssid: \n" + String(ssid));
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    put(".");
  }

  debug("\nWiFi connected successfully to ssid: "+String(ssid));
  putln("The IP Address of this module is: \n"+WiFi.localIP().toString());

  debug("Connecting to server "+server.toString()+" on port",port);
  if (wc.connect(server,port)) {
    delay(5000);
    putln("Connected!"); // wait for initializing
  }
  else {
    putln("Connection failed.");
    for(;;);
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
    debug("Message = "+message+", http code = "+String(http_code)+", response = "+response);
    putln("I sent: \n"+message);
    put("Server responded:\n");
    put(response+" ("+String(http_code)+")");
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off
    delay(5000);
    http.end();
    yield();
  }
  
}

