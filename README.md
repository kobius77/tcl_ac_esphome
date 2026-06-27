# ESPhome implementation of TCL OEM air conditioners
### (slightly improved)

## Motivation

The AC's internal "room temperature" sensor is very coarse — it only reports in steps of ~0.8°C. This was probably designed for a dumb on/off with fixed deadband controller, not precise regulation.  Besides, sitting inside the Box, usually mounted close to the ceiling, what this sensor mesuares probably is all but 'room temperature' for most situations.  And after all, the esp in the module combined with the HA climate integration doesn't do anything different than the IR-remote. BONUS: this makes the machine regulate it's rpm ('modulate') nicely.


## Custom dynamic Thermostat (kind of)

Implementing an external sensor had long been an idea, so with a little vibe we can now trick the AC's internal thermostat to act like a custom hysteresis-free controller that uses an **external Home Assistant temperature sensor** for regulation:

### How it works

This ESPhome firmware implements **proportional target injection**: instead of telling the AC to target the user's set temperature, it computes a virtual target that makes the AC modulate proportionally to the real room temperature.

**Formula**: `sent_target = internal_temp + user_target - external_temp`

The AC sees `gap = internal - sent = external - user_target`, so it modulates its compressor RPM proportionally to how far the real room is from the target. No on/off cycling — the AC runs at the minimum necessary RPM to hold the setpoint.

### Key features

- **External sensor regulation** — uses one or two HA temperature sensors (configurable as primary, fallback, or average)
- **Preserves user's target** — the temperature you set in the UI is never overwritten by the code
- **Rate-limited sends** — regulation commands are sent at most every 30s to avoid spamming the UART
- **Runtime toggles** — display and beeper can be switched on/off from HA without reflashing
- **Fallback mode** — if the external sensor is unavailable, falls back to the AC's internal reading

### Exposed sensors

| Entity | Description |
|--------|-------------|
| Temperature Ext1 | External sensor #1 (primary) |
| Temperature Ext2 | External sensor #2 (secondary) |
| Temperature Int | AC's internal coarse reading (debugging) |
| Temperature Cmd | Computed target sent to the AC |
| VeskaAC Power | Power draw from smart plug |

### Result

The AC now modulates smoothly, keeping the room temperature steady at the setpoint while running at the minimum necessary compressor RPM. No more temperature swings, no more on/off cycling.

---

# From Original Project:

### Implemented:
- Split system modes (auto, cool, dry, fan only, heat)
- Fan modes (mute, min, min-mid, mid, mid-high, high, turbo)
- Indoor unit temperature
- Target temperature
- Swing mode (only h/v, and visualization only)

### Tested on:
- Royal Clima rci-pf40hn
- Lennox LI036CI-180P432
- SunWind SW-18
- Kesser Split 12000/BTU
- Veska VSK-12000BTU (likely TCL TAC-12chsa/xa73i, as the Kesser above)

### Unsuccessfully tested:
- TCL-12chsa/tpg

### Tuya Module 32001-000140
The [original WiFi-Module](https://github.com/user-attachments/assets/f1888a35-ba68-4869-9790-71ff8c572931) is an ESP8266 and it's original Tuya firmware can be replaced with Tasmota or esphome. It's case is easy to open and [solderpads for serial connection](https://github.com/user-attachments/assets/4515421f-4346-4248-aba7-d4db3886ac40) are available.
The wired UART for the connection to the AC's mainboard uses tx_pin: GPIO15 / rx_pin: GPIO13

### Donation: 
- kaspi kz (outside Russia) 4400430344051161
- sber (Russia) 2202205034977568
