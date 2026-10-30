# ESPHome custom components

A collection of ESPHome external components for hardware that has no (complete) support in ESPHome itself:
Remeha boilers over CAN bus, Itho ventilation boxes over I²C, MiLight/MiBoxer lights over 2.4 GHz radio,
LD2410S mmWave presence sensors, DCF77 radio clocks and an I²C sniffer for reverse engineering.

Every component ships with a complete, working example configuration.

| Component | What it does | Bus / hardware | ESPHome platforms | Example |
| --------- | ------------ | -------------- | ----------------- | ------- |
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
  components: [itho]  # or mi, ld2410s, dcf77, i2c_sniffer
  refresh: 0s
```

Pin a specific version by appending a tag or a **full** commit SHA
(`github://hencou/esphome_components@1.1.6`); short SHAs are rejected by GitHub. After changing the
reference, run *Clean Build Files* once so ESPHome discards the old checkout.

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
