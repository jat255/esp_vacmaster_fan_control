Vacmaster AM201R Remote Replacement — Build Plan

Background

The fan (Vacmaster Aero AM201R Air Mover) ships with remote model RC49A (FCC ID WN2KL-RC49A, PCB silkscreen "DBY-RC49A V2.0"), a Suzhou Cleva product. Key facts gathered from the FCC filing's public exhibits:

Frequency: 433.92 MHz, certified under Part 15.231 (periodic/on-demand short-range transmitter).

No microcontroller: the remote is a single 8-pin glob-top (chip-on-board) encoder IC — the die is wire-bonded directly to the PCB and covered in unmarked black epoxy. There is no readable part number, so the exact chip and its voltage spec are unknown. The pin count and lack of address/DIP-switch pins are consistent with a common fixed-code OOK encoder family (e.g. EV1527-style), but this is an inference, not a confirmed datasheet match.

4 buttons (SW1-SW4) on the board, matching the 3-speed + power functions described in the product manual.

Antenna: a printed PCB trace loop (not the visible wound spring, which is a battery contact).

Battery: a 12V A27/L828 alkaline cell — low capacity and not a battery most households keep spares of, which is the likely real cause of "burns through batteries" complaints.

No published reverse-engineering of this exact remote or "DBY-RC49A" board exists (checked rtl_433 issues, Flipper Zero sub-GHz community DBs, general searches) — the codes must be captured directly from your unit.

Parts list

Everything below (except items you already own) can be ordered from DigiKey in a single cart — no need to split the order across vendors. There is no true single combined Tx/Rx module for plain OOK 433MHz on a Grove connector (M5Stack and Seeed both sell the transmitter and receiver as two separate modules; the only single-chip Grove transceiver on the market is a LoRa module, which uses an incompatible radio protocol), but since both halves ship from the same distributor this doesn't add an extra order.

2026-08-14 DigiKey - order confirmation

Item

DigiKey part

Price

Link

M5Stack AtomS3 Lite (ESP32-S3)

C124

$7.68

digikey.com

M5Stack Unit RF433T (SYN115 transmitter)

U114

$4.50

digikey.com

M5Stack Unit RF433R (SYN531R receiver) — used only during capture, keep as a spare afterward

U113

$4.50

digikey.com

Alligator-clip-to-alligator-clip test leads (12-pack)

Adafruit 1592

$3.95

digikey.com

Small precision screwdriver — likely already owned

—

—

Opens the remote's case (2 small screws)

USB-C cable + USB wall adapter — likely already owned

—

—

Permanent, always-plugged-in power for the finished device

A 9V battery — already owned

—

—

Powers the remote during capture; no holder needed, clip directly to its snap terminals

Total new spend: ~$20.63. No battery holder is needed: the plan powers the remote from a 9V battery clipped directly to its snap terminals (see Step 1 below), which doesn't need a holder at all. If a holder-based fallback (8 AA cells, ~12V) ends up being necessary, that's cheap to add later, but isn't part of the base plan.

No soldering or breadboard required anywhere in this build — everything connects via the Grove connector and alligator clips.

Safety notes

No mains AC voltage is involved anywhere in this project — the fan's 120V wiring is never opened; only the low-voltage remote and USB-powered ESP32 are handled.

A 9-12V battery pack cannot shock you through skin (shock requires roughly 30V+). RF exposure is negligible — the FCC filing includes a SAR test exclusion, meaning transmit power is too low to require exposure testing.

The one real (minor) risk is shorting the battery pack — if the two alligator clips touch each other or another conductor while connected, a 9V battery in particular can get noticeably hot within seconds. Keep clips spread apart once connected, rest the pack on a non-flammable surface, and disconnect between tests rather than leaving it clipped in. If anything gets warm, disconnect immediately.

Inspect any old spare batteries for corrosion/leakage before handling.

Step 1 — Capture the remote's codes

Open the remote's case (2 small screws) to expose the A27 battery contacts.

Power it with a 9V battery, clipped directly to the remote's battery contacts (matching any + / − markings molded into the battery compartment). The exact chip is unidentifiable (unmarked glob-top), but the device's rated operating voltage is known — it's the OEM's own 12V spec — so anything at or below 12V is within its normal designed range, not a guess. A 9V battery is simpler to wire than stacking enough AAs to hit exactly 12V, and there's no reason to creep up from a much lower voltage first: below-rated voltage risks a weak or absent transmission (a functional problem), not damage, so there's nothing gained by starting low.Press a button and watch for LED1 on the board to light (this is the best available indicator of activity, though its exact function isn't confirmed from the FCC photos).

If nothing happens at 9V, the board may need closer to its full 12V — try 8 AA cells in series (~12V) next, staying at or under the OEM spec rather than exceeding it.

Wire the M5Stack RF433R to the AtomS3 Lite via its included Grove cable.

In the Arduino IDE, install the ESP32 board support package and the rc-switch library, then flash the library's ReceiveDemo example sketch (update it to use the Grove data pin as the receive pin).

Open the Serial Monitor. With the remote powered per step 2, press each of the 4 buttons in turn and record what's printed for each: the decimal code, protocol number, and pulse length.

Sanity check: the 4 codes should share a long common prefix (the chip's fixed ID) and differ only in the last few bits (the per-button data bits). This is the expected signature of an EV1527-style encoder.

If nothing decodes: rc-switch only recognizes a fixed set of known protocols. Fall back to the RTL-SDR you already own, using rtl_433 (brew install rtl_433 on your Mac) in analyze mode (rtl_433 -f 433.92M -A) to capture and manually inspect the raw pulse timing instead. No new hardware purchase is needed for this fallback.

Once all 4 codes are captured, the harvested remote board no longer needs power — set it aside.

Step 2 — Build the replacement device

Swap the RF433R for the RF433T on the same Grove cable/port.

Flash ESPHome (not the Arduino sketch) as the permanent firmware. Use board esp32-s3-devkitc-1 with the Arduino framework and variant: esp32s3 (this is the board/variant ESPHome's own device directory lists for the AtomS3 Lite).A remote_transmitter component on the Grove data pin, driving the RF433T.

Four actions (or a fan entity with 3 speeds + off) that call remote_transmitter.transmit_rc_switch_raw, one per captured code from Step 1.

A binary_sensor on the AtomS3 Lite's built-in button (GPIO41) using on_multi_click to map single/double/triple-click and long-press to the same 4 actions — this gives physical control with no extra wiring.

(Optional) drive the onboard RGB LEDs (4x WS2812 on GPIO35) to show the current speed as a status color.

Add the device to Home Assistant — the ESPHome integration auto-discovers it and exposes the entities for app/automation control.

Mount the AtomS3 Lite + RF433T near the fan, plugged into a USB wall adapter for continuous power (no battery in the finished device).

Step 3 — Test

Verify each of the 4 replayed codes reproduces the correct behavior on the fan (power on/off, speed 1/2/3), both via a physical button-press sequence on the AtomS3 Lite and via each Home Assistant entity.

Check transmit range/reliability from the AtomS3 Lite's intended mounting location before permanently placing it.

Known risks / fallbacks

Protocol doesn't match a known rc-switch type: fall back to RTL-SDR + rtl_433 for raw capture (Step 1.7) — no extra hardware needed, same procedure otherwise.

Range from RF433T is too short at the install location: reposition the AtomS3 Lite closer to the fan's receiver window, or check for interference sources.
