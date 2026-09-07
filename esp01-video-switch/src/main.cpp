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

// %SSID% and %IP% are substituted at request time in buildPage() so this
// stays in sync with the AP credentials/address without duplicating them.
const char PAGE_TEMPLATE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>Video Switch</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  :root {
    --bg: #12151b;
    --panel: #1b1f27;
    --panel-raised: #20242e;
    --border: #2c313d;
    --border-soft: #262b35;
    --text: #e7e9ee;
    --text-dim: #8992a3;
    --text-faint: #565d6c;
    --accent: #ff8a3d;
    --cm1: #3ddc84; --cm1-dim: #1d4a34;
    --cm2: #4d9fff; --cm2-dim: #1d3a5c;
    --cm3: #ffcc33; --cm3-dim: #5c4d1a;
    --danger: #ff5470;
    --mono: ui-monospace, "SF Mono", "Cascadia Code", Consolas, "Liberation Mono", monospace;
    --sans: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  }
  * { box-sizing: border-box; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: var(--sans);
    min-height: 100vh;
    margin: 0;
    padding: 40px 16px 56px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 22px;
  }
  .wrap { width: 100%; max-width: 460px; display: flex; flex-direction: column; gap: 18px; }
  header { text-align: center; }
  .eyebrow { font-family: var(--mono); font-size: 11px; letter-spacing: .12em; text-transform: uppercase; color: var(--accent); margin-bottom: 4px; }
  h1 { margin: 0; font-size: 24px; font-weight: 600; }
  .panel { background: var(--panel); border: 1px solid var(--border); border-radius: 16px; padding: 22px; display: flex; flex-direction: column; gap: 22px; }
  .readout { display: flex; align-items: baseline; justify-content: space-between; flex-wrap: wrap; gap: 12px; padding-bottom: 18px; border-bottom: 1px solid var(--border-soft); }
  .readout-value { display: flex; align-items: baseline; gap: 8px; font-family: var(--mono); }
  .readout-value .num { font-size: 44px; font-weight: 600; line-height: 1; }
  .readout-value .unit { font-size: 14px; color: var(--text-dim); }
  .readout-label { font-size: 10.5px; letter-spacing: .1em; text-transform: uppercase; color: var(--text-faint); margin-bottom: 5px; }
  .chan-chip { font-family: var(--mono); font-size: 12px; font-weight: 600; letter-spacing: .05em; padding: 6px 12px; border-radius: 999px; border: 1px solid transparent; display: inline-flex; align-items: center; gap: 6px; white-space: nowrap; }
  .chan-chip .dot { width: 7px; height: 7px; border-radius: 50%; }
  .chan-chip.cm1 { background: var(--cm1-dim); color: var(--cm1); }
  .chan-chip.cm1 .dot { background: var(--cm1); }
  .chan-chip.cm2 { background: var(--cm2-dim); color: var(--cm2); }
  .chan-chip.cm2 .dot { background: var(--cm2); }
  .chan-chip.cm3 { background: var(--cm3-dim); color: var(--cm3); }
  .chan-chip.cm3 .dot { background: var(--cm3); }
  .chan-chip.custom { background: var(--panel-raised); color: var(--text-dim); }
  .chan-chip.custom .dot { background: var(--text-faint); }
  .stroke-label { font-size: 10.5px; letter-spacing: .1em; text-transform: uppercase; color: var(--text-faint); margin-bottom: 8px; }
  .stroke-track { position: relative; height: 30px; border-radius: 8px; background: var(--panel-raised); border: 1px solid var(--border-soft); }
  .band { position: absolute; top: 0; bottom: 0; }
  .band.cm1 { left: 0%; width: 40%; background: linear-gradient(180deg, var(--cm1-dim), transparent); border-right: 1px solid var(--border-soft); border-radius: 8px 0 0 8px; }
  .band.gap1 { left: 40%; width: 5%; background: repeating-linear-gradient(45deg, var(--border-soft) 0 4px, transparent 4px 8px); }
  .band.cm2 { left: 45%; width: 15%; background: linear-gradient(180deg, var(--cm2-dim), transparent); border-left: 1px solid var(--border-soft); border-right: 1px solid var(--border-soft); }
  .band.gap2 { left: 60%; width: 5%; background: repeating-linear-gradient(45deg, var(--border-soft) 0 4px, transparent 4px 8px); }
  .band.cm3 { left: 65%; width: 35%; background: linear-gradient(180deg, var(--cm3-dim), transparent); border-left: 1px solid var(--border-soft); border-radius: 0 8px 8px 0; }
  .pointer { position: absolute; top: -8px; width: 2px; height: 46px; background: var(--accent); left: 20%; transform: translateX(-1px); transition: left .3s ease; }
  .pointer::after { content: ""; position: absolute; top: -6px; left: 50%; transform: translateX(-50%); width: 0; height: 0; border-left: 6px solid transparent; border-right: 6px solid transparent; border-top: 7px solid var(--accent); }
  @media (prefers-reduced-motion: reduce) { .pointer { transition: none; } }
  .stroke-ticks { display: flex; justify-content: space-between; font-family: var(--mono); font-size: 10px; color: var(--text-faint); margin-top: 6px; }
  .chan-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
  .chan-btn { font-family: var(--sans); background: var(--panel-raised); border: 1px solid var(--border); border-radius: 12px; padding: 14px 8px 12px; color: var(--text); cursor: pointer; display: flex; flex-direction: column; align-items: center; gap: 5px; }
  .chan-btn:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
  .chan-btn .name { font-weight: 600; font-size: 14px; }
  .chan-btn .range { font-family: var(--mono); font-size: 10.5px; color: var(--text-faint); }
  .chan-btn.cm1.active { border-color: var(--cm1); background: var(--cm1-dim); }
  .chan-btn.cm1.active .name { color: var(--cm1); }
  .chan-btn.cm2.active { border-color: var(--cm2); background: var(--cm2-dim); }
  .chan-btn.cm2.active .name { color: var(--cm2); }
  .chan-btn.cm3.active { border-color: var(--cm3); background: var(--cm3-dim); }
  .chan-btn.cm3.active .name { color: var(--cm3); }
  .manual { display: flex; flex-direction: column; gap: 9px; padding-top: 18px; border-top: 1px solid var(--border-soft); }
  .manual-row { display: flex; gap: 9px; }
  .manual-field { position: relative; flex: 1; display: flex; align-items: center; }
  .manual-field input { width: 100%; background: var(--panel-raised); border: 1px solid var(--border); border-radius: 10px; color: var(--text); font-family: var(--mono); font-size: 16px; padding: 11px 40px 11px 12px; }
  .manual-field input:focus-visible { outline: 2px solid var(--accent); outline-offset: 1px; }
  .manual-field .unit-tag { position: absolute; right: 12px; font-family: var(--mono); font-size: 11px; color: var(--text-faint); pointer-events: none; }
  .apply-btn { font-family: var(--sans); font-weight: 600; font-size: 14px; background: var(--accent); color: #1a0f04; border: none; border-radius: 10px; padding: 0 20px; cursor: pointer; }
  .apply-btn:focus-visible { outline: 2px solid #fff; outline-offset: 2px; }
  .hint { font-size: 11.5px; color: var(--text-faint); }
  .msg { font-family: var(--mono); font-size: 12px; color: var(--danger); min-height: 15px; }
  .devcard { display: grid; grid-template-columns: repeat(2, 1fr); gap: 1px; background: var(--border-soft); border: 1px solid var(--border-soft); border-radius: 12px; overflow: hidden; font-family: var(--mono); }
  .devcard div { background: var(--panel); padding: 10px 13px; display: flex; flex-direction: column; gap: 3px; }
  .devcard .k { font-size: 9.5px; letter-spacing: .08em; text-transform: uppercase; color: var(--text-faint); }
  .devcard .v { font-size: 12.5px; color: var(--text); word-break: break-all; }
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="eyebrow">ESP-01S &middot; Video Switch Controller</div>
    <h1>Signal Select</h1>
  </header>

  <div class="panel">
    <div class="readout">
      <div>
        <div class="readout-label">Current pulse width</div>
        <div class="readout-value">
          <span class="num" id="pwmNum">--</span>
          <span class="unit">&micro;s @ 50 Hz</span>
        </div>
      </div>
      <div class="chan-chip custom" id="chanChip"><span class="dot"></span><span id="chanLabel">--</span></div>
    </div>

    <div>
      <div class="stroke-label">Switching stroke &mdash; 1000&ndash;2000 &micro;s</div>
      <div class="stroke-track">
        <div class="band cm1"></div>
        <div class="band gap1"></div>
        <div class="band cm2"></div>
        <div class="band gap2"></div>
        <div class="band cm3"></div>
        <div class="pointer" id="pointer"></div>
      </div>
      <div class="stroke-ticks"><span>1000</span><span>1400</span><span>1600</span><span>2000</span></div>
    </div>

    <div class="chan-grid">
      <button class="chan-btn cm1" id="btnCm1" onclick="press('/cm1')">
        <span class="name">CM1</span><span class="range">1000&ndash;1400 &micro;s</span>
      </button>
      <button class="chan-btn cm2" id="btnCm2" onclick="press('/cm2')">
        <span class="name">CM2</span><span class="range">1450&ndash;1600 &micro;s</span>
      </button>
      <button class="chan-btn cm3" id="btnCm3" onclick="press('/cm3')">
        <span class="name">CM3</span><span class="range">1650&ndash;2000 &micro;s</span>
      </button>
    </div>

    <div class="manual">
      <div class="stroke-label">Manual pulse width</div>
      <div class="manual-row">
        <div class="manual-field">
          <input type="number" id="customUs" min="1000" max="2000" step="1" value="1200" inputmode="numeric" aria-label="Custom pulse width in microseconds">
          <span class="unit-tag">&micro;s</span>
        </div>
        <button class="apply-btn" onclick="applyCustom()">Apply</button>
      </div>
      <div class="msg" id="msg"></div>
      <div class="hint">Accepts 1000&ndash;2000. Values inside a gap (1400&ndash;1450, 1600&ndash;1650) still drive the line but won't register as a preset channel.</div>
    </div>
  </div>

  <div class="devcard">
    <div><span class="k">Network</span><span class="v">%SSID%</span></div>
    <div><span class="k">Address</span><span class="v">%IP%</span></div>
  </div>
</div>

<script>
function pct(us) { return ((us - 1000) / 1000) * 100; }
function render(data) {
  document.getElementById('pwmNum').textContent = data.pwm;
  document.getElementById('customUs').value = data.pwm;
  document.getElementById('pointer').style.left = pct(data.pwm) + '%';
  var chan = data.channel;
  var chip = document.getElementById('chanChip');
  chip.className = 'chan-chip ' + chan.toLowerCase();
  document.getElementById('chanLabel').textContent = chan === 'custom' ? 'Custom' : chan + ' active';
  ['Cm1', 'Cm2', 'Cm3'].forEach(function(name) {
    document.getElementById('btn' + name).classList.toggle('active', chan === name.toUpperCase());
  });
}
function press(path) {
  document.getElementById('msg').textContent = '';
  fetch(path).then(function(r) { return r.json(); }).then(render);
}
function applyCustom() {
  var v = document.getElementById('customUs').value;
  var msg = document.getElementById('msg');
  fetch('/set?us=' + encodeURIComponent(v))
    .then(function(r) { return r.json(); })
    .then(function(data) {
      if (data.error) {
        msg.textContent = data.error;
      } else {
        msg.textContent = '';
        render(data);
      }
    });
}
window.onload = function() {
  fetch('/status').then(function(r) { return r.json(); }).then(render);
};
document.getElementById('customUs').addEventListener('keydown', function(e) {
  if (e.key === 'Enter') applyCustom();
});
</script>
</body>
</html>
)HTML";

String buildPage() {
  String html = FPSTR(PAGE_TEMPLATE);
  html.replace("%SSID%", AP_SSID);
  html.replace("%IP%", WiFi.softAPIP().toString());
  return html;
}

void handleRoot() {
  server.send(200, "text/html", buildPage());
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
