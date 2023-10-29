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
A I2C sniffer implementation for a ESP32 module, using the code from https://github.com/ozarchie/I2C-sniffer. Will dump all the camptured packets from the I2C interface to the ESPhome logging console.
