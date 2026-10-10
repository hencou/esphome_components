# ESPHome custom components

A collection of ESPHome external components for hardware that has no (complete) support in ESPHome itself:
Remeha boilers over CAN bus, Itho ventilation boxes over I²C, MiLight/MiBoxer lights over 2.4 GHz radio,
LD2410S mmWave presence sensors, DCF77 radio clocks and an I²C sniffer for reverse engineering.

Every component ships with a complete, working example configuration.

| Component | What it does | Bus / hardware | ESPHome platforms | Example |
| --------- | ------------ | -------------- | ----------------- | ------- |
| [`remeha`](#remeha---remeha-boilers-over-can-bus) | Read and write Remeha boiler data and parameters | CAN bus (service connector), ESP32 | `sensor`, `text_sensor`, `number`, `select`, `climate` | [`example_remeha.yaml`](example_remeha.yaml) |
| [`itho`](#itho---itho-daalderop-ventilation-over-i2c) | Control and monitor an Itho ventilation box | I²C, ESP32 | `fan`, `select`, `sensor` | [`example_itho.yaml`](example_itho.yaml) |
| [`mi`](#mi---milight--miboxer-lights-over-24-ghz) | MiLight/MiBoxer bulb control and remote emulation | nRF24L01+ over SPI, ESP8266/ESP32 | `light`, `button` | [`example_milight.yaml`](example_milight.yaml) |
| [`ld2410s`](#ld2410s---hlk-ld2410s-mmwave-presence-sensor) | HLK-LD2410S presence sensor incl. calibration | UART | `binary_sensor`, `sensor`, `text_sensor`, `number`, `select`, `switch`, `button` | [`example_ld2410s.yaml`](example_ld2410s.yaml) |
| [`dcf77`](#dcf77---atomic-clock-time-source) | Time source from the DCF77 atomic clock signal | GPIO receiver module | `time` | [`example_dcf77.yaml`](example_dcf77.yaml) |
| [`i2c_sniffer`](#i2c_sniffer---i2c-bus-sniffer) | Dumps all captured I²C traffic to the ESPHome log | 2 GPIO pins, ESP32 | – | [`example_i2csniffer.yaml`](example_i2csniffer.yaml) |

## Installation

Add the component(s) you need as an external component:

```yaml
external_components:
- source: github://hencou/esphome_components
  components: [remeha]   # or itho, mi, ld2410s, dcf77, i2c_sniffer
  refresh: 0s
```

Pin a specific version by appending a tag or a **full** commit SHA
(`github://hencou/esphome_components@1.1.6`); short SHAs are rejected by GitHub. After changing the
reference, run *Clean Build Files* once so ESPHome discards the old checkout.

## `remeha` - Remeha boilers over CAN bus

Reads the service bus of a Remeha boiler (developed on a Tzerra Ace Matic) and exposes temperatures,
status, modulation, error history and the writable installer parameters as ESPHome entities. The service
bus is an RJ12 connector:

| Pin | RJ12   |
| --- | ------ |
| 1   | CAN H  |
| 2   | CAN L  |
| 3   | NC     |
| 4   | GND    |
| 5   | NC     |
| 6   | 24V    |

An Olimex ESP32-EVB is convenient because it has a CAN transceiver on board; with a 24 V to 5 V buck
converter on pin 6 the board can be powered from the service bus itself. (Use at your own risk.)

```yaml
canbus:
- platform: esp32_can
  id: can1
  tx_pin: GPIO5
  rx_pin: GPIO35
  can_id: 0
  bit_rate: 1000KBPS

remeha:
  canbus_id: can1
  user_level: 2          # 0 = no authentication, 1 = GUEST, 2 = SERVICE, 3
  auth_key: !secret remeha_auth_key
  boot_delay: 10s        # optional, delay before the boiler is addressed
  min_write_level: 2     # optional, refuse writes below this verified level
```

Broadcast (PDO) data such as temperatures, status and modulation is available without authentication.
The protected service objects and all parameter writes need an authentication handshake, which uses a
fixed 32-bit key word that is **not** included in this repository — supply it via `auth_key`
(with or without `0x`, always read as hexadecimal), which is required for `user_level` 1 and up:

```yaml
# secrets.yaml
remeha_auth_key: 0x........
```

With `user_level: 0` the handshake is skipped and `auth_key` may be omitted; only broadcast data is then
available. Authentication only succeeds when the boiler reports the requested level as the effective
access level, and writes are refused while that verified level is below `min_write_level`.

How the component talks to the boiler:

- On boot it sends NMT reset/start and reads object `0x4004` on the standard SDO channel (`0x601/0x581`).
  The boiler answers with a channel number, from which the request/response ids follow
  (1 → `0x241/0x1C1`, 2 → `0x341/0x2C1`), so several clients (Recom, eThermostat, this component) can be
  on the bus at the same time. A channel change triggers re-authentication.
- Poll entries come from the entities you configure: every `sensor`, `number`, `select` and `climate`
  entity registers its own object. A full round is read back to back (next request after the previous
  response) once a minute, with single keep-alive reads in between so the authorisation does not expire.
- If the CAN controller goes bus-off, the component requests TWAI recovery, restarts the driver and
  re-runs the boot sequence — ESPHome's `esp32_can` does not recover by itself.

- Zone parameters (room/night/holiday setpoint, heating curve, zone mode, time program, fireplace mode)
  are arrays of up to ten zones, DHW parameters arrays of up to ten circuits. The `number`, `select` and
  `climate` platforms take `zone:` and `dhw_circuit:` (both default `1`) to address another zone.
- Writes are checked against the configured range, step and object size before a frame goes out; the
  boiler's abort stays as the second line of defence and the write status reports `REJECTED: …`.

Entities: `sensor` (flow/return/outside/DHW temperatures, water pressure, modulation, pump speed, status
and error codes, appliance type, …), `text_sensor` (status/substatus text, write status, last error and
error history slots),
`number` (room, DHW comfort/reduced, night, holiday and anti-legionella setpoints, heating curve slope,
room sensor calibration, summer/winter threshold), `select` (zone mode, time program, CH/DHW enable,
anti-legionella, fireplace mode) and `climate`.

The error history walks the error arrays (`0x1003` holds the entry count in subindex 0 and a
`{code, category}` struct per entry, `0x2004` the matching customer codes) every fifteen minutes. The
`last_error` text sensor shows the most recent entry as `E:03.52 (customer code 22)`; `error_1` …
`error_5` hold the same five newest entries in compact form, one per slot, so they can be put in a table.

## `itho` - Itho Daalderop ventilation over I²C

An ESPHome port of [arjenhiemstra/ithowifi](https://github.com/arjenhiemstra/ithowifi): speaks I²C to the
Itho box, exposing it as a `fan` plus a `select` for the ventilation mode and sensors for error code,
startup counter, operation time, fan setpoint/speed and the optional on-board SHT30 temperature and
humidity.

```yaml
itho:
  syssht30: disable      # or the SHT30 variant on the Itho board
  # syssht30_address: 0x44
  # sda: SDA
  # scl: SCL

fan:
- platform: itho
  name: Fan

select:
- platform: itho
  name: Stand

sensor:
- platform: itho
  update_interval: 8s
  error:
    name: Error
  fan_speed:
    name: Fan speed
```

[`example_itho.yaml`](example_itho.yaml) also contains a standalone PID controller that drives the box
from the humidity sensor, including shower detection.

## `mi` - MiLight / MiBoxer lights over 2.4 GHz

An ESPHome implementation of [Sidoh's MiLight Hub](https://github.com/sidoh/esp8266_milight_hub/). Usable
as a standalone hub replacement or behind a wall switch, in which case the switch keeps working without
WiFi. State updates go out over the native ESPHome API.

```yaml
mi:
  ce_pin: D2
  csn_pin: D8
  packet_repeats: 50          # optional
  listen_repeats: 20          # optional, packets needed to accept a remote command
  state_flush_interval: 5000  # optional, ms between state reports

light:
- platform: mi
  name: Living room
  mi_device_id: 0xAB01
  group_id: 1
  remote_type: rgb_cct        # rgb_cct, rgb, cct, rgbw, fut089, fut091, fut020, s2
  default_transition_length: 0s
```

There is also a `button` platform for `pair`/`unpair`. Button press behaviour in
[`example_milight.yaml`](example_milight.yaml): short press toggles, long press fades to maximum,
two short presses select night mode, three short presses white mode.

## `ld2410s` - HLK-LD2410S mmWave presence sensor

Full support for the LD2410S over UART: presence, target distance, firmware version, per-gate thresholds,
detection range and delays, output mode and the auto-calibration routine (`button`, plus a
`calibration_progress` sensor and `has_calibration_running` binary sensor).

```yaml
uart:
  id: uart_bus
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 115200

ld2410s:
  uart_id: uart_bus
```

Note: switch off minimal output mode before starting calibration, otherwise there is no progress
reporting. Based on the work of [@NovakIrs](https://github.com/NovakIrs).

## `dcf77` - Atomic clock time source

Decodes the time broadcast by the DCF77 transmitter and provides it as an ESPHome `time` source, so a
device can keep accurate time without network access.

```yaml
time:
- platform: dcf77
  pin: GPIO4
  update_interval: 30min
```

## `i2c_sniffer` - I²C bus sniffer

Passively captures I²C traffic (based on [ozarchie/I2C-sniffer](https://github.com/ozarchie/I2C-sniffer))
and dumps every packet to the ESPHome log — handy for reverse engineering devices such as the Itho box.

```yaml
i2c_sniffer:
```
