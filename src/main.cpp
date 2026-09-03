#include <Adafruit_NeoPixel.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"

#define WS2812B_PIN 8
#define PIXEL_COUNT 25
#define STATUS_LED_PIN 10

Adafruit_NeoPixel grid(PIXEL_COUNT, WS2812B_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>01Space C3-RGB</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: sans-serif; text-align: center; margin-top: 60px; }
  button {
    font-size: 24px;
    padding: 20px 40px;
    margin: 10px;
    border: none;
    border-radius: 10px;
    color: white;
  }
  .yellow { background: #d4b800; }
  .blue   { background: #0057d4; }
  .green  { background: #1a9c1a; }
  .white  { background: #d40000; }
</style>
</head>
<body>
  <h1>01Space C3-RGB</h1>
  <button class="yellow" onclick="fetch('/yellow')">Yellow</button>
  <button class="blue" onclick="fetch('/blue')">Blue</button>
  <button class="green" onclick="fetch('/green')">Green</button>
  <button class="white" onclick="fetch('/white')">White</button>
</body>
</html>
)HTML";

void fillGrid(uint8_t r, uint8_t g, uint8_t b) {
  grid.fill(grid.Color(r, g, b));
  grid.show();
}

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleYellow() {
  fillGrid(255, 200, 0);
  server.send(200, "text/plain", "yellow");
}

void handleBlue() {
  fillGrid(0, 0, 255);
  server.send(200, "text/plain", "blue");
}

void handleGreen() {
  fillGrid(0, 255, 0);
  server.send(200, "text/plain", "green");
}

void handleWhite() {
  fillGrid(255, 0, 0);
  server.send(200, "text/plain", "white");
}

void setup() {
  Serial.begin(115200);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  grid.begin();
  grid.setBrightness(60);
  grid.clear();
  grid.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(STATUS_LED_PIN, HIGH);

  server.on("/", handleRoot);
  server.on("/yellow", handleYellow);
  server.on("/blue", handleBlue);
  server.on("/green", handleGreen);
  server.on("/white", handleWhite);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
