# SuperFan v2 — ESPHome firmware

ESPHome port of `main_1v0.ino`, controlled from Home Assistant through the native API (no MQTT).

## Hardware

| Function | GPIO |
|---|---|
| Relay speed 1 / 2 / 3 (active HIGH, mutually exclusive) | 19 / 23 / 33 |
| NeoPixel status LED (1 × WS2812, GRB) | 27 |
| Push button | 21 (GPIO 25 driven LOW as the button's ground) |

Board: ESP32 WROOM-32 (`esp32dev`), ESP-IDF framework.

## Modes

The `Modo` select in Home Assistant and the physical button both change the mode. The device always powers up in **Off**.

| Mode | Behaviour |
|---|---|
| Off | Relays off. BLE stays connected. |
| Auto HR | 3 heart-rate zones → 3 speeds. The zones shift with temperature: gym sensor from HA blended with CORE skin temperature (weighted) when CORE is connected. Cold → zones + shift, hot → zones − shift, linear in between. |
| Temp. alvo | Holds a CORE core-temperature target: off below the target, then one speed per band, with hysteresis, a minimum hold time before stepping down and an optional look-ahead on the temperature trend. If CORE is lost the LED blinks and the speed is held; after `core_timeout_min` (5 min) it switches to Manual at the same speed. |
| Manual | Speed 1/2/3 chosen by hand. Stays until the mode is changed. |
| Remoto | An external app controls the speed with `fan.set_percentage` (0/33/66/100). |

**No time-based reverts:** changing the fan from HA while in Off, Auto HR or Temp. alvo switches to Manual (or Off if the fan is turned off). In Remoto, fan commands are accepted as the app's control.

### Button

- Short press: Off → Auto HR → Temp. alvo → Manual 1 → 2 → 3 → Off. In Remoto: take over in Manual at the current speed.
- Long press (≥ 1.5 s): Off.

### LED

- Mode change: 1.5 s flash in the mode colour (Off white, Auto HR cyan, Temp. alvo magenta, Manual amber, Remoto blue).
- Then: speed 1 green, 2 orange, 3 red. Speed 0 shows a faint glow in the mode colour. Blinking in the mode colour means the sensor has no data.

### Remote control from an app

```yaml
- service: select.select_option
  target: { entity_id: select.superfan_modo }
  data: { option: "Remoto" }
- service: fan.set_percentage
  target: { entity_id: fan.superfan_ventoinha }
  data: { percentage: 66 }
```

## BLE sensors

- Up to 2 heart-rate straps (Heart Rate Service `0x180D` / `0x2A37`).
- CORE body-temperature sensor (CoreTemp BLE service v2.2, `00002100-5B1E-4347-B07C-97B514DAE121`): core temperature, skin temperature, quality/state, relayed HR, heat strain index, and battery level.
- Active HR source priority: strap 1 → strap 2 → HR relayed by CORE (whichever has data from the last 10 s).

## Setup

1. Copy `secrets.example.yaml` to `secrets.yaml` and fill it in.
2. In `superfan.yaml` → `substitutions`, set the MAC addresses (`mac_fita_1`, `mac_fita_2`, `mac_core`) and the gym temperature entity (`ha_temp_ginasio`). Delete the `fita_2` blocks if you only have one strap.
3. `esphome run superfan.yaml`
4. Dashboard: paste `ha-card.yaml` into a manual card. It uses only built-in cards. Check the `entity_id`s after adopting the device.

Find MACs with nRF Connect, or in the ESPHome logs (`esp32_ble_tracker`). CORE advertises the service UUID above; don't rely on its name.

## Status

Only the YAML syntax has been checked. `esphome config` and a compile have not been run yet.
