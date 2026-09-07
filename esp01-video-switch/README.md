# ESP-01 Video Switch Controller

Targets the original ESP-01 (ESP8266, 1MB flash). It uses GPIO1 (the
hardware serial TX pin) as a status LED, which only lines up with the
onboard blue LED's wiring on the plain ESP-01 — on an ESP-01S that LED
is on GPIO2 instead, so if you're on an ESP-01S you'd need to move the
LED define back to GPIO2's neighbor or wire an external LED to GPIO1.

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
- An onboard LED that blinks out the active channel number (1/2/3), so you
  can tell what's selected without opening the page. See below.
- The last pulse width applied survives reboot/power loss — it's saved to
  flash and reloaded at boot instead of always starting on CM1.

## Wiring

- Signal output: **GPIO2** on the ESP-01 -> video switch module's PWM input.
- Common ground between the ESP-01 and the video switch module.
- Power the ESP-01 from a stable 3.3V source (a plain USB-serial adapter's
  3.3V pin usually cannot supply enough current under WiFi load).

GPIO2 was chosen over GPIO0 because GPIO0 doubles as a flash-mode strap pin
at boot; GPIO2 is safer to drive as a general-purpose output.

## Status LED (GPIO1 / TX)

This firmware doesn't use hardware serial at runtime, so GPIO1 (TX) is
repurposed to drive the ESP-01's onboard LED as a channel indicator:

- **1 short blink, then 1s pause, repeat** — CM1 active
- **2 short blinks, then 1s pause, repeat** — CM2 active
- **3 short blinks, then 1s pause, repeat** — CM3 active
- **LED off** — a custom value that falls in a gap between channels
  (1400-1450 or 1600-1650)

Because TX is in use, there's no serial output to monitor, and you can't
send commands to the board over the USB-TTL adapter's TX/RX lines while
it's running normally (only during flashing, when GPIO0 is grounded and
the bootloader owns the pins). The ROM bootloader briefly flickers the LED
with boot noise at power-on before `setup()` takes over — that's normal.

## Persistence

Every time a channel button or the manual Apply sets a new pulse width, it's
written to the ESP8266's flash-emulated EEPROM (skipped if it's the same
value already stored, so repeat presses don't wear the flash). On boot,
`setup()` reads that value back and resumes on it; a first boot with nothing
saved yet (or corrupted/erased flash) falls back to CM1.

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
   There's no serial output to check afterward (TX is now the status LED)
   — the AP coming up is confirmed by seeing `VideoSwitch` in your WiFi
   list and the LED blinking once flashing is done and it reboots.

## Using it

1. Connect to the WiFi network `VideoSwitch` (password `videoswitch123`,
   set in `src/main.cpp` if you want to change it).
2. Open `http://192.168.4.1/` in a browser.
3. Press CM1 / CM2 / CM3 to switch inputs, or enter a custom microsecond
   value (1000-2000) and press Apply.
