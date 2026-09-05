#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// Signal output pin. ESP-01 only breaks out GPIO0 and GPIO2 besides
// TX/RX; GPIO2 has an internal pull-up and is the safer pick since
// GPIO0 doubles as the flash-mode strap pin.
#define PWM_PIN 2

#define PWM_MIN 1000
#define PWM_MAX 2000

#define CM1_MIN 1000
#define CM1_MAX 1400
#define CM2_MIN 1450
#define CM2_MAX 1600
#define CM3_MIN 1650
#define CM3_MAX 2000

// Preset pulse widths sent when a channel button is pressed (roughly
// mid-stroke of each channel's window).
#define CM1_VALUE 1200
#define CM2_VALUE 1525
#define CM3_VALUE 1825

// Access point credentials. This device is meant to be connected to
// directly, so there is no station WiFi config to manage.
static const char *AP_SSID = "VideoSwitch";
static const char *AP_PASSWORD = "videoswitch123"; // 8+ chars required for WPA2

Servo pwmOut;
ESP8266WebServer server(80);

int currentPwm = CM1_VALUE;

const char *channelForPwm(int us) {
  if (us >= CM1_MIN && us <= CM1_MAX) return "CM1";
  if (us >= CM2_MIN && us <= CM2_MAX) return "CM2";
  if (us >= CM3_MIN && us <= CM3_MAX) return "CM3";
  return "custom";
}

void setPwm(int us) {
  us = constrain(us, PWM_MIN, PWM_MAX);
  currentPwm = us;
  pwmOut.writeMicroseconds(us);
}

String statusJson() {
  String json = "{\"pwm\":";
  json += currentPwm;
  json += ",\"channel\":\"";
  json += channelForPwm(currentPwm);
  json += "\"}";
  return json;
}

const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>Video Switch</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: sans-serif; text-align: center; margin-top: 40px; }
  button {
    font-size: 20px;
    padding: 16px 28px;
    margin: 8px;
    border: none;
    border-radius: 10px;
    color: white;
    cursor: pointer;
  }
  .cm1 { background: #1a9c1a; }
  .cm2 { background: #0057d4; }
  .cm3 { background: #d4b800; }
  #status { font-size: 22px; margin: 24px 0; }
  #status span { font-weight: bold; }
  .custom { margin-top: 20px; }
  .custom input {
    font-size: 18px;
    padding: 8px;
    width: 100px;
    text-align: center;
  }
  .custom button {
    background: #555;
  }
  #msg { min-height: 20px; color: #c00; }
</style>
</head>
<body>
  <h1>Video Switch</h1>
  <div id="status">Current PWM: <span id="pwmval">--</span> us (<span id="chan">--</span>)</div>

  <div>
    <button class="cm1" onclick="press('/cm1')">CM1<br>1000-1400us</button>
    <button class="cm2" onclick="press('/cm2')">CM2<br>1450-1600us</button>
    <button class="cm3" onclick="press('/cm3')">CM3<br>1650-2000us</button>
  </div>

  <div class="custom">
    <input type="number" id="customus" min="1000" max="2000" step="1" value="1200">
    <button onclick="applyCustom()">Apply</button>
  </div>
  <div id="msg"></div>

<script>
function refresh(data) {
  document.getElementById('pwmval').textContent = data.pwm;
  document.getElementById('chan').textContent = data.channel;
  document.getElementById('customus').value = data.pwm;
  document.getElementById('msg').textContent = '';
}
function press(path) {
  fetch(path).then(function(r) { return r.json(); }).then(refresh);
}
function applyCustom() {
  var v = document.getElementById('customus').value;
  fetch('/set?us=' + encodeURIComponent(v))
    .then(function(r) { return r.json(); })
    .then(function(data) {
      if (data.error) {
        document.getElementById('msg').textContent = data.error;
      } else {
        refresh(data);
      }
    });
}
window.onload = function() {
  fetch('/status').then(function(r) { return r.json(); }).then(refresh);
};
</script>
</body>
</html>
)HTML";

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleStatus() {
  server.send(200, "application/json", statusJson());
}

void handleCm1() {
  setPwm(CM1_VALUE);
  server.send(200, "application/json", statusJson());
}

void handleCm2() {
  setPwm(CM2_VALUE);
  server.send(200, "application/json", statusJson());
}

void handleCm3() {
  setPwm(CM3_VALUE);
  server.send(200, "application/json", statusJson());
}

bool isNumeric(const String &s) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}

void handleSet() {
  if (!server.hasArg("us") || !isNumeric(server.arg("us"))) {
    server.send(400, "application/json", "{\"error\":\"invalid value\"}");
    return;
  }
  int us = server.arg("us").toInt();
  if (us < PWM_MIN || us > PWM_MAX) {
    server.send(400, "application/json",
                "{\"error\":\"value must be between 1000 and 2000\"}");
    return;
  }
  setPwm(us);
  server.send(200, "application/json", statusJson());
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);

  pwmOut.attach(PWM_PIN, PWM_MIN, PWM_MAX);
  setPwm(CM1_VALUE);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/cm1", handleCm1);
  server.on("/cm2", handleCm2);
  server.on("/cm3", handleCm3);
  server.on("/set", handleSet);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
