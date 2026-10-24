# Project Context

## Device
- **ESP**: 192.168.1.78 (ESP8266, Tuya 32001-000140 module)
- **Config**: veskaac.yml (device name: veskaac)

## AC Behavior
- Internal "room temperature" sensor is coarse (~24.9°C, ~24.1°C) — designed for dumb deadband logic, imprecise and uncomfortable
- When cooling, temperature goes *down*
- Users set a target temp in the UI — never change what the user sees

## Primary Goal
Replace the AC's internal temperature sensor with `sensor.buro_gang_temperature_2` (external Home Assistant sensor) so the AC targets real room temp instead of its coarse internal reading.

## Implementation — Custom Thermostat
The ESP firmware implements its own hysteresis controller, bypassing the AC's internal thermostat:

1. **Override `current_temperature`** — uses the external HA sensor instead of the AC's coarse internal reading
2. **Preserve user's target** — the temperature the user sets in the HA UI is never overwritten by the code; the AC's reported target is ignored once the user has made a selection
3. **Proportional target injection**: Sends a computed target to the AC so that the AC's internal modulation works correctly:
   - `sent_target = internal_temp + user_target - external_temp`
   - The AC sees gap = `internal - sent` = `external - user_target` → modulates proportionally to real room temp
4. **Cooling**: When `ext_temp > target` → send computed target (modulated). When `ext_temp <= target` → send target=31°C (compressor stops)
5. **Heating**: Reversed logic
6. **Sends commands only on state transition** — avoids spamming the UART every 450ms loop
7. **Falls back to internal sensor** if the external HA sensor is unavailable

## Protocol Reference
- https://github.com/adaasch/AC-hack — documents the UART protocol used by this AC
- The `half_degree` bit exists in the set command struct but is not supported by this AC model (3-beep error + reset to whole degree)
- `disp` bit: 1 = show target temp, 0 = show internal coarse temp (~23°C). Not a true "display off".

## Relevant Files
- `components/tcl_climate/tcl_climate.cpp` — main UART protocol logic, temperature parsing at line 412
- `components/tcl_climate/tcl_climate.h` — struct definitions for command frames
- `veskaac.yml` — device config, already defines `sensor.buro_gang_temperature_2` and `sensor.veskaac_power`
- `tcl-ac.yml` — alternative config using GitHub component source

## Exposed Sensors
- `ext_temp_sensor` (config option) — external HA sensor used for regulation
- `internal_temp` (config option) — publishes the AC's internal coarse sensor reading as a HA sensor for debugging
- `power_sensor` (config option) — reads power draw (W) from a HA sensor like a smart plug for observation
