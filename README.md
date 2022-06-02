# esphome_components
This is my collection of ESPhome custom componenents

# 1. MiLight for ESPhome
This is a fork of Sidoh's Milight Hub: https://github.com/sidoh/esp8266_milight_hub/
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

More detailed description about setting up MiLight devices: https://github.com/hencou/esphome-milight

# 2. DCF77 for ESPhome
The DCF77 library adds the ability to read and decode the atomic time broadcasted by the DCF77 radiostation.

See the attached example "example_dcf77.yaml"

# 3. Itho for ESPhome
This is an ESPhome implementation of Itho Wifi module: https://github.com/arjenhiemstra/ithowifi.

See attached example "example_itho.yaml"
