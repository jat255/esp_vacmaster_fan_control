# Vacmaster Fan RF Remote → ESPHome / Home Assistant

Replaces the RF433.92MHz remote for a **Vacmaster Aero AM201R Air Mover** (remote
model RC49A) with an M5Stack AtomS3 Lite + RF433T transmitter Unit, controlled
through ESPHome and exposed in Home Assistant.

See [`implementation_plan.md`](implementation_plan.md) for the full background,
parts list, and original build plan this project followed.

## Hardware

- M5Stack AtomS3 Lite (ESP32-S3)
- M5Stack Unit RF433T (SYN115 transmitter) — permanent, drives the fan
- M5Stack Unit RF433R (SYN531R receiver) — used only to capture the original
  remote's codes; not needed in the finished device
- USB-C cable + 5V wall adapter for permanent power

### Wiring

Both Units connect via Grove cable to the AtomS3 Lite's single Grove port
(GPIO1/GPIO2). Only one of the two GPIOs carries the Unit's actual data line
(the 4-pin HY2.0 connector is GND / 5V / NC / signal); the working pin for
this build was **GPIO2**.

## Captured codes

Captured with the RF433R + `receive.ino` sketch, pressing each button on the
original remote. All four share a common 20-bit prefix (`213760`) with a
one-hot bit for the button, confirming a fixed-code (non-rolling) encoder —
protocol 1 in both `rc-switch` and ESPHome's `rc_switch` component
(350µs pulse length).

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
| `receive.ino` | Arduino sketch (RF433R + [rc-switch](https://github.com/sui77/rc-switch)) used once to capture the four codes above. Not needed after capture. |
| `transmit.ino` | Arduino sketch used to validate the RF433T could replay the captured codes before committing to ESPHome. Kept for reference/debugging. |
| `vacmaster-rf-control.yaml` | The permanent ESPHome firmware config — WiFi, API, and four `button` entities that replay the captured codes via `remote_transmitter.transmit_rc_switch_raw`. |
| `dashboard_card.yaml` | Lovelace card YAML (header + 2x2 button grid) for the Home Assistant dashboard. |
| `implementation_plan.md` | Original research/build plan, including FCC filing details on the remote being replaced. |

## Setup

### 1. Capture (only needed if re-deriving codes for a different remote)

1. Wire the RF433R to the AtomS3 Lite's Grove port.
2. Flash `receive.ino` (Arduino IDE, ESP32 board package + `rc-switch`
   library installed, board **M5AtomS3**).
3. Open Serial Monitor at 9600 baud, press each remote button, record the
   decimal code / bit length / protocol / pulse length.

### 2. Deploy the ESPHome firmware

1. Copy `vacmaster-rf-control.yaml` into your ESPHome config directory.
2. Create a `secrets.yaml` alongside it (not committed to this repo) with:
   ```yaml
   wifi_ssid: "your-ssid"
   wifi_password: "your-password"
   vacmaster_rf_capture__encryption_key: "<generate with: openssl rand -base64 32>"
   ```
3. Wire the RF433T to the AtomS3 Lite's Grove port (GPIO2).
4. `esphome run vacmaster-rf-control.yaml` (USB for first flash, OTA after).
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
