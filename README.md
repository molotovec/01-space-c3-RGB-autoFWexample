Project is at ~/Documents/01_space_c3_rgb-esp32-c3fh4-rgb, built and flashed to an ESP32-C3 board (/dev/cu.usbmodem144401, confirmed via esptool).

What's set up (using the fivebyfive/01SPACE_C3_RGB pin reference — this board has a 5×5 grid of 25 WS2812B LEDs on GPIO8, not a single LED):
- platformio.ini — esp32-c3-devkitm-1 board, Arduino framework, native USB-CDC flags, Adafruit NeoPixel lib dep.
- src/secrets.h — your WiFi credentials (gitignored, not committed — see setup below).
- src/secrets.h.example — template to copy from.
- src/main.cpp — connects as a WiFi station, runs a WebServer on port 80 serving a page with Yellow/Blue/Green/White buttons; each button hits /yellow, /blue, /green, /white and fills all 25 pixels with that color.

## Setup

1. Copy the secrets template and fill in your WiFi credentials:
   ```
   cp src/secrets.h.example src/secrets.h
   ```
   Then edit `src/secrets.h` and set `WIFI_SSID` / `WIFI_PASSWORD`. This file is gitignored so your credentials never get committed.
2. Build and upload: `cd ~/Documents/01_space_c3_rgb-esp32-c3fh4-rgb && python3 -m platformio run -t upload`
3. Watch serial for the IP: `python3 -m platformio device monitor -p /dev/cu.usbmodem144401 -b 115200`
4. Open `http://<that-ip>/` in a browser on the same network and press a button.
