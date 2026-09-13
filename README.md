# esphome_components
This is my collection of ESPhome custom componenents

# 1. MiLight for ESPhome
This is a ESPhome implementation of Sidoh's Milight Hub: https://github.com/sidoh/esp8266_milight_hub/
This project can be used as standalone ESP Hub replacement or combined with a switch configuration to use behind a wall switch.
* This version is deployed by ESPHome
* RGB_CCT Lights can be controlled MiLight remote or a wall push switch.
* Wall switches/light combi will remains working even without WIFI.
* Status updates will be send by the native ESPHome API.

Button press functions in provided example "example_milight.yaml": 
* Short press: light on/off
* Long press: fade light in to max brightness
* 2 short presses: night mode
* 3 short presses: white mode

# 2. DCF77 for ESPhome
The DCF77 library adds the ability to read and decode the atomic time broadcasted by the DCF77 radiostation.

See the attached example "example_dcf77.yaml"

# 3. Itho for ESPhome
This is an ESPhome implementation of Itho Wifi module: https://github.com/arjenhiemstra/ithowifi.

See attached example "example_itho.yaml". This example provides also an integrated standalone PID controller to drive the Itho box, with values from the integrated humidity sensor.

# 4. I2C sniffer for ESPhome
A I2C sniffer implementation for a ESP32 module, using the code from https://github.com/ozarchie/I2C-sniffer. Will dump all the captured packets from the I2C interface to the ESPhome logging console.

# 5. ESPHome CANbus implementation for use with Remeha boilers
A CANbus implementation with use of standard ESPHome components to read CANbus messages from the service bus of Remeha Tzerra Ace Matic boiler. The service bus needs to be connected with a RJ12 connector. 
The connection is as follows:

| Pin  | RJ12   |
| ---- | ------ |
| 1    | CAN H  |
| 2    | CAN L  |
| 3    | NC     |
| 4    | GND    |
| 5    | NC     | 
| 6    | 24V    |

I am using a Olimex ESP32-EVP board for this, because this board has already a CANbus connection onboard. By a 24V to 5V buck converter inbetween the ESP32 and service connector pin 6 the ESP32 can be power supplied from the Remeha service bus.
(Use at your own risk)

The broadcast (PDO) data such as temperatures, status and modulation is available without authentication. The protected service objects and all parameter writes require an authentication handshake with the boiler, which uses a fixed 32-bit key word that is not included in this repository. Supply it through the `auth_key` option, which is required for `user_level` 1 and up:

```yaml
# secrets.yaml
remeha_auth_key: 0x........

# device config
remeha:
  canbus_id: can1
  user_level: 2  # 0=no authentication, 1=GUEST, 2=SERVICE
  auth_key: !secret remeha_auth_key
  min_write_level: 2  # refuse parameter writes below this verified access level
```

The value may be written with or without the `0x` prefix and is always interpreted as hexadecimal. With `user_level: 0` the handshake is skipped entirely and `auth_key` can be omitted; only the broadcast data is then available.

Authentication only succeeds when the boiler reports the requested level as the effective access level, and parameter writes are refused while that verified level is below `min_write_level` (default 2).
