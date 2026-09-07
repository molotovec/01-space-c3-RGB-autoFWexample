# ESP-01 Video Switch Controller

Works on both the original ESP-01 and the ESP-01S (same flash size and
GPIO0/GPIO2 pinout).

Standalone firmware for an ESP-01 (ESP8266) driving a video switch module's
PWM select line. The switch reads a 50Hz, 1000-2000us servo-style PWM signal
to pick between 3 inputs:

- **CM1**: 1000-1400us
- **CM2**: 1450-1600us
- **CM3**: 1650-2000us

The board runs as its own WiFi access point (no router/station config
needed) and serves a small webpage with:

- Three buttons — one per channel — that jump to a preset pulse width in
  that channel's window and show the resulting value.
- A live status line showing the current PWM value (in us) and which
  channel it falls in.
- A custom pulse-width input (1000-2000us) with an Apply button for manual
  tuning within or across channel windows.

## Wiring

- Signal output: **GPIO2** on the ESP-01 -> video switch module's PWM input.
- Common ground between the ESP-01 and the video switch module.
- Power the ESP-01 from a stable 3.3V source (a plain USB-serial adapter's
  3.3V pin usually cannot supply enough current under WiFi load).

GPIO2 was chosen over GPIO0 because GPIO0 doubles as a flash-mode strap pin
at boot; GPIO2 is safer to drive as a general-purpose output.

Note: many ESP-01S boards tie their onboard blue status LED to GPIO2. If
yours does, the LED will flicker in time with the PWM pulses — harmless,
just cosmetic, and it won't affect the signal seen by the video switch.

## Build & flash

ESP-01 has no onboard USB, so flashing requires an external USB-TTL adapter:

1. Wire adapter TX -> ESP-01 RX, adapter RX -> ESP-01 TX, GND -> GND.
2. Pull GPIO0 to GND to enter flash mode, then power/reset the board.
3. Build and upload:
   ```
   cd esp01-video-switch
   python3 -m platformio run -t upload
   ```
   Set `upload_port` in `platformio.ini` if it isn't auto-detected.
4. Release GPIO0 (or power-cycle without it pulled low) to run normally.
5. Watch serial for the AP details:
   ```
   python3 -m platformio device monitor -b 115200
   ```

## Using it

1. Connect to the WiFi network `VideoSwitch` (password `videoswitch123`,
   set in `src/main.cpp` if you want to change it).
2. Open `http://192.168.4.1/` in a browser.
3. Press CM1 / CM2 / CM3 to switch inputs, or enter a custom microsecond
   value (1000-2000) and press Apply.
