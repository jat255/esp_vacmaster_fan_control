# Vacmaster Fan RF Remote → ESPHome / Home Assistant

Replaces the RF433.92MHz remote for a **[Vacmaster Aero AM201R Air Mover](https://us.cleva.com/products/vacmaster-aero-air-mover-fitness-fan-am201r)** (remote
model RC49A) with an M5Stack AtomS3 Lite + RF433T transmitter Unit, controlled
through ESPHome and exposed in Home Assistant.

See [`implementation_plan.md`](implementation_plan.md) for the full background,
parts list, and original build plan this project followed.

> **Disclaimer:** This project is an independent, unofficial DIY integration
> and is not affiliated with, endorsed by, or sponsored by Vacmaster or its
> parent company in any way. "Vacmaster" and related product names are
> trademarks of their respective owners, used here only to identify the
> hardware this project is compatible with.

## Hardware

| Item | Notes | Image |
|---|---|---|
| M5Stack [AtomS3 Lite](https://docs.m5stack.com/en/core/AtomS3%20Lite) (ESP32-S3) | The brains of the build — runs ESPHome, connects to WiFi/Home Assistant, and drives the RF433T over its Grove port. The only piece that needs to be flashed/configured; the RF433T and RF433R are passive radio modules with no firmware of their own. | ![AtomS3 Lite](https://static-cdn.m5stack.com/resource/docs/products/core/AtomS3%20Lite/img-dc6432b6-fd9b-4066-9a4d-49786503d1a3.webp) |
| M5Stack Unit [RF433T](https://docs.m5stack.com/en/unit/rf433_t) (SYN115 transmitter) | Permanent, drives the fan | ![RF433T](https://static-cdn.m5stack.com/resource/docs/products/unit/rf433_t/rf433_t_01.webp) |
| M5Stack Unit [RF433R](https://docs.m5stack.com/en/unit/rf433_r) (SYN531R receiver) | Used only to capture the original remote's codes; not needed in the finished device if you just copy the values here (assuming the codes are not unique to each fan — I haven't tested that) | ![RF433R](https://static-cdn.m5stack.com/resource/docs/products/unit/rf433_r/rf433_r_01.webp) |
| USB-C cable | For permanent power |  |
| 9V Battery + alligator clips | For temporarily powering the original remote, if its battery is dead and you want to confirm what codes are sent by your remote |  |

### Wiring

Both units connect via the Grove cable included with the receiver/transmitter 
to the AtomS3 Lite's single Grove port (GPIO1/GPIO2), so no soldering or manual
connections are required. Only one of the two GPIOs
carries the Unit's actual data line (the 4-pin HY2.0 connector is
GND / 5V / NC / signal); the working pin for the RF433T is **GPIO2** (the RF433R
receives on *GPIO1*).

## Captured codes

For reference, these are the codes that were captured with the RF433R & 
`arduino/receive/receive.ino` sketch, pressing each button on the
original remote. All four share a common 20-bit prefix (`213760`) with a
one-hot bit for the button, confirming a fixed-code (non-rolling) encoder —
this corresponds to protocol 1 in both `rc-switch` and ESPHome's `rc_switch`
component (350µs pulse length).

| Button   | Decimal | Binary (24-bit)             |
|----------|---------|------------------------------|
| Power    | 213761  | `000000110100001100000001`  |
| Speed 1 (low)    | 213768  | `000000110100001100001000`  |
| Speed 2 (medium) | 213764  | `000000110100001100000100`  |
| Speed 3 (high)   | 213762  | `000000110100001100000010`  |

"Power" is a **toggle**, not a distinct on/off code — the remote (and this
replacement) can't know the fan's actual power state, only that pressing it
flips whatever state it's currently in.

## Files

| File | Purpose |
|---|---|
| `arduino/receive/receive.ino` | Arduino sketch (RF433R + [rc-switch](https://github.com/sui77/rc-switch)) used once to capture the four codes above. Not needed after capture. |
| `arduino/transmit/transmit.ino` | Arduino sketch used to validate the RF433T could replay the captured codes before committing to ESPHome. Kept for reference/debugging. |
| `vacmaster-rf-control.yaml` | The permanent ESPHome firmware config — WiFi, API, and four `button` entities that replay the captured codes via `remote_transmitter.transmit_rc_switch_raw`. |
| `dashboard_card.yaml` | Lovelace card YAML (header + 2x2 button grid) for the Home Assistant dashboard. |
| `implementation_plan.md` | Original research/build plan, including some FCC filing information on the remote being replaced. |

## Setup

### 0. Install Arduino IDE

- Available from https://www.arduino.cc/en/software/

### 1. Capture (only needed if re-deriving codes for a different remote)

1. Wire the RF433R to the AtomS3 Lite's Grove port.
2. Flash `receive.ino` (Arduino IDE, ESP32 board package + `rc-switch`
   library installed, board **M5AtomS3**).
3. Open Serial Monitor at 9600 baud, press each remote button, record the
   decimal code / bit length / protocol / pulse length.

### 1.5. Test the codes with Arduino (optional)

Before committing to the ESPHome firmware, you can validate the captured
codes and wiring with the RF433T directly from the Arduino IDE:

1. Swap the RF433R for the RF433T on the same Grove cable/port.
2. Flash `arduino/transmit/transmit.ino` (same board/library setup as
   above).
3. Open Serial Monitor at 115200 baud. Send a digit on the Serial Monitor 
   (`0`-`3`) to fire the matching command and confirm the fan responds correctly.

### 2. Deploy the ESPHome firmware

1. Copy `vacmaster-rf-control.yaml` into your ESPHome config directory.
2. Create a `secrets.yaml` alongside it (not committed to this repo) with:
   ```yaml
   wifi_ssid: "your-ssid"
   wifi_password: "your-password"
   vacmaster_rf_capture__encryption_key: "<generate with: openssl rand -base64 32>"
   ```
3. Wire the RF433T to the AtomS3 Lite's Grove port (GPIO2).
4. `esphome run vacmaster-rf-control.yaml` (USB for first flash, OTA after) — this can also be done via the ESPHome app inside a Home Assistant installation.
5. Adopt the device in Home Assistant (Settings → Devices & Services →
   ESPHome should auto-discover it).

### 3. Add the dashboard card

In Home Assistant: Edit Dashboard → Add Card → **Manual** → paste the
contents of `dashboard_card.yaml` → Save. Update the `entity:` IDs first if
your device/area naming differs (check Developer Tools → States, filter by
`button.`).

## Key implementation notes

- **`protocol: 1`** in ESPHome's `rc_switch` component is bit-for-bit
  identical to `rc-switch`'s protocol 1 (verified against ESPHome's
  [`rc_switch_protocol.cpp`](https://github.com/esphome/esphome/blob/dev/esphome/components/remote_base/rc_switch_protocol.cpp) source) — no protocol mismatch.
- **`repeat: { times: 10, wait_time: 0us }`** on each `transmit_rc_switch_raw`
  action is required — without it, ESPHome sends only a single frame, which
  the fan's receiver ignores. This was the actual fix after initial "buttons
  do nothing" debugging (RMT buffer size and `carrier_duty_percent` format
  were both ruled out as red herrings along the way).
- `carrier_duty_percent: 100%` is required for OOK/ASK 433MHz transmission
  (vs. ~50% for IR), so the RF433T is driven as a plain digital signal with
  no sub-carrier modulation.
