# Vacmaster AM201R Remote Replacement — Build Plan

A story about being annoyed at having to buy a weird 12V A27/L828 battery every
six months to use the remote control of my fan.

## Background

The fan (Vacmaster Aero AM201R Air Mover) ships with remote model RC49A (FCC ID WN2KL-RC49A, PCB silkscreen "DBY-RC49A V2.0"), a Suzhou Cleva product. Key facts gathered from the FCC filing's public exhibits:

- *Frequency:* 433.92 MHz, certified under Part 15.231 (periodic/on-demand short-range transmitter).
- *No microcontroller:* the remote is a single 8-pin glob-top (chip-on-board) encoder IC — the die is wire-bonded directly to the PCB and covered in unmarked black epoxy. There is no readable part number, so the exact chip and its voltage spec are unknown. The pin count and lack of address/DIP-switch pins are consistent with a common fixed-code OOK encoder family (e.g. EV1527-style), but this is an inference, not a confirmed datasheet match.
- *Control surface:* 4 buttons (SW1-SW4) on the board, matching the 3-speed + power functions described in the product manual.
- *Antenna:* Antenna: a printed PCB trace loop (not the visible wound spring, which is a battery contact).
- *Battery:* a 12V A27/L828 alkaline cell — low capacity and not a battery most households keep spares of, which is why the remote burns through a $6 battery every 6 months or so.

No published reverse-engineering of this exact remote or "DBY-RC49A" board were found (checked rtl_433 issues, Flipper Zero sub-GHz community DBs, general searches) — the codes must be captured directly from the unit.

## Parts list

Everything below (except items you probably already own) can be ordered from DigiKey and the project will not require soldering (all plug and play). I could not find a true single combined Tx/Rx module for plain OOK 433MHz on a Grove connector (M5Stack and Seeed both sell the transmitter and receiver as two separate modules; the only single-chip Grove transceiver on the market is a LoRa module, which uses an incompatible radio protocol), but since both halves ship from the same distributor this doesn't add an extra order.

| Item | DigiKey part | Price | Link / Notes |
|---|---|---|---|
| M5Stack AtomS3 Lite (ESP32-S3) | C124 | $7.68 | digikey.com |
| M5Stack Unit RF433T (SYN115 transmitter) | U114 | $4.50 | digikey.com |
| M5Stack Unit RF433R (SYN531R receiver, used only during capture, keep as a spare afterward) | U113 | $4.50 | digikey.com |
| Alligator-clip-to-alligator-clip test leads (12-pack) | Adafruit 1592 | $3.95 | digikey.com |
| Small precision screwdriver (likely already owned) | — | — | Opens the remote's case (3 small screws) |
| USB-C cable + USB wall adapter (likely already owned) | — | — | Permanent, always-plugged-in power for the finished device |
| A 9V battery (already owned) | — | — | Powers the remote during capture; no holder needed, clip directly to its snap terminals |

Total new spend: ~$20.63. No battery holder is needed: the plan powers the remote from a 9V battery clipped directly to its snap terminals (see Step 1 below), which doesn't need a holder at all. If a holder-based fallback (8 AA cells, ~12V) ends up being necessary, that's cheap to add later, but isn't part of the base plan.

No soldering or breadboard required anywhere in this build — everything connects via the Grove connector and alligator clips.

## Safety notes

No mains AC voltage is involved anywhere in this project — the fan's 120V wiring is never opened; only the low-voltage remote and USB-powered ESP32 are handled.

A 9V battery pack cannot shock you through skin (shock requires roughly 30V+). RF exposure is negligible — the FCC filing includes a SAR test exclusion, meaning transmit power is too low to require exposure testing.

The one real (minor) risk is shorting the battery pack — if the two alligator clips touch each other or another conductor while connected, a 9V battery in particular can get noticeably hot within seconds. Keep clips spread apart once connected, rest the pack on a non-flammable surface, and disconnect between tests rather than leaving it clipped in. If anything gets warm, disconnect immediately.

Inspect any old spare batteries for corrosion/leakage before handling.

## Step 1 — Capture the remote's codes

Open the remote's case (2 small screws) to expose the A27 battery contacts.

Power it with a 9V battery, clipped directly to the remote's battery contacts (matching any + / − markings molded into the battery compartment). The original battery specced is 12V, but 9V appears to work for this purpose. While the battery is connected, press a button and watch for LED1 on the board to light to confirm it's transmitting.

Wire the M5Stack RF433R to the AtomS3 Lite via its included Grove cable.

In the Arduino IDE, install the M5Stack library and the rc-switch library, then flash the `arduino/receive/receive.ino` sketch from this repo.

Open the Serial Monitor. With the remote powered per step 2, press each of the 4 buttons in turn and record what's printed for each: the decimal code, protocol number, and pulse length.

Sanity check: the 4 codes should share a long common prefix (the chip's fixed ID) and differ only in the last few bits (the per-button data bits). This is the expected signature of an EV1527-style encoder.

Once all 4 codes are captured, the harvested remote board no longer needs power — disconnect the battery and set it aside.

## Step 2 — Test the replacement device with Arduino

Swap the RF433R for the RF433T on the same Grove cable/port on the AtomS3 Lite.

Flash the `arduino/transmit/transmit.ino` sketch from this repo (same Arduino IDE setup as Step 1, no additional libraries needed).

Open the Serial Monitor at 115200 baud. It should print a ready message and a key for each of the 4 commands (power, speed 1/2/3).

With the fan powered on and in range, send each digit (0-3) in turn through the Serial Monitor and confirm the fan responds correctly: power toggles on/off, and each speed command switches to the expected speed.

If a command doesn't register, double check the code values in `transmit.ino` against what was captured in Step 1, and confirm the RF433T's data wire lands on GPIO2 as wired (since there's only one cable, this should be the case by default, but see the comment in `transmit.ino`).

Once all 4 codes are confirmed working from the Arduino sketch, this validates the codes and wiring before moving on to the permanent ESPHome firmware in Step 3.

## Step 3 — Flash the final device for use with ESPHome

Flash ESPHome (not the Arduino sketch) as the permanent firmware (this can be done in Home Assistant, if you have it available). Use board type esp32-s3-devkitc-1 with the Arduino framework and variant: esp32s3 (this is the board/variant ESPHome's own device directory lists for the AtomS3 Lite). A `remote_transmitter` component will be used on the Grove data pin, driving the RF433T.

Four actions (or a fan entity with 3 speeds + off) that call `remote_transmitter.transmit_rc_switch_raw`, one per captured code from Step 1.

(Optional, not implemented in `vacmaster-rf-control.yaml`) A `binary_sensor` on the AtomS3 Lite's built-in button (GPIO41) using on_multi_click to map single/double/triple-click and long-press to the same 4 actions — this gives physical control with no extra wiring.

(Optional, not implemented in `vacmaster-rf-control.yaml`) drive the onboard RGB LEDs (4x WS2812 on GPIO35) to show the current speed as a status color.

Add the device to Home Assistant — the ESPHome integration auto-discovers it and exposes the entities for app/automation control.

Mount the AtomS3 Lite + RF433T near the fan, plugged into a USB wall adapter or computer for continuous power (no battery in the finished device).

## Step 4 — Test

Verify each of the 4 replayed codes reproduces the correct behavior on the fan (power on/off, speed 1/2/3), both via a physical button-press sequence on the AtomS3 Lite and via each Home Assistant entity.

Check transmit range/reliability from the AtomS3 Lite's intended mounting location before permanently placing it.
