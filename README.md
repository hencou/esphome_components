# esphome_components
This is my collection of ESPhome custom componenents

# 1. MiLight for ESPhome
This is a fork of Sidoh's Milight Hub: https://github.com/sidoh/esp8266_milight_hub/
Instead of using a central hub this project aims to be used behind every wall switch.
* This version is deployed by ESPHome
* RGB_CCT Lights can be controlled MiLight remote or a wall push switch.
* Wall switches will remains working even without WIFI.
* Status updates will be send by the native ESPHome API.

Button press functions:
* Short press: light on/off
* Long press: fade light in to max brightness
* 2 short presses: night mode
* 3 short presses: white mode

Below the original text of Sidoh's Milight Hub with slight modifications:

# esp8266_milight_hub/switch
This is a replacement for a Milight/LimitlessLED remote/gateway hosted on an ESP8266. Leverages [Henryk Plötz's awesome reverse-engineering work](https://hackaday.io/project/5888-reverse-engineering-the-milight-on-air-protocol).

[Milight bulbs](https://www.amazon.com/Mi-light-Dimmable-RGBWW-Spotlight-Smart/dp/B01LPRQ4BK/r) are cheap smart bulbs that are controllable with an undocumented 2.4 GHz protocol. In order to control them, you either need a [remote](https://www.amazon.com/Mi-light-Dimmable-RGBWW-Spotlight-Smart/dp/B01LCSALV6/r?th=1) ($13), which allows you to control them directly, or a [WiFi gateway](http://futlight.com/productlist.aspx?typeid=125) ($30), which allows you to control them with a mobile app or a [UDP protocol](https://github.com/Fantasmos/LimitlessLED-DevAPI).

[This guide](http://blog.christophermullins.com/2017/02/11/milight-wifi-gateway-emulator-on-an-esp8266/) on my blog details setting one of these up.

## Why this is useful

1. Both the remote and the WiFi gateway are limited to four groups. This means if you want to control more than four groups of bulbs, you need another remote or another gateway. This project allows you to control 262,144 groups (4*2^16, the limit imposed by the protocol).
2. This project exposes a nice ESPHome API to control your bulbs.

## Supported remotes

The following remotes can be emulated:

Support has been added for the following [bulb types](http://futlight.com/productlist.aspx?typeid=101):

Model #|Name|Compatible Bulbs
-------|-----------|----------------
|FUT096|RGB/W|<ol><li>FUT014</li><li>FUT016</li><li>FUT103</li>|
|FUT005<br/>FUT006<br/>FUT007</li></ol>|CCT|<ol><li>FUT011</li><li>FUT017</li><li>FUT019</li></ol>|
|FUT098|RGB|Most RGB LED Strip Controlers|
|FUT020|RGB|Some other RGB LED strip controllers|
|FUT092|RGB/CCT|<ol><li>FUT012</li><li>FUT013</li><li>FUT014</li><li>FUT015</li><li>FUT103</li><li>FUT104</li><li>FUT105</li><li>Many RGB/CCT LED Strip Controllers</li></ol>|
|FUT091|CCT v2|Most newer dual white bulbs and controllers|
|FUT089|8-zone RGB/CCT|Most newer rgb + dual white bulbs and controllers|

Other remotes or bulbs, but have not been tested.

## What you'll need

1. An ESP8266. I used a Wemos D1 Mini.
2. A NRF24L01+ module (~$3 on ebay). Alternatively, you can use a LT8900.
3. Some way to connect the two (7 female/female dupont cables is probably easiest).
4. 10 uF capacitor between power supply and NRF24L01+
5. HLK-PM03 3V3 Power supply, or HLK-PM01 5V to support RCWL-0516 radar sensor (optional)

## Installing

#### Connecting GPIO

I used a Wemos D1 mini because it's very small to fit behind a wall switch.
* Use free GPIO inputs as input from wall switch to control groups 1-3. LOW is active state. Use a push switch or touch sensor like TTP223, solder jumper A on TTP223 to achieve active LOW output.
* Use free GPIO input for a DS18B20 temperature sensor, or RCWL-0516 radar sensor

#### Connect the NRF24L01+ / LT8900

This project is compatible with both NRF24L01 and LT8900 radios. LT8900 is the same model used in the official MiLight devices. NRF24s are a very common 2.4 GHz radio device, but require software emulation of the LT8900's packet structure. As such, the LT8900 is more performant.

Both modules are SPI devices and should be connected to the standard SPI pins on the ESP8266.

##### NRF24L01+

[This guide](https://www.mysensors.org/build/connect_radio#nrf24l01+-&-esp8266) details how to connect an NRF24 to an ESP8266. By default GPIO 4 for CE and GPIO 15 for CSN are used.

<img src="https://user-images.githubusercontent.com/40266/47967518-67556f00-e05e-11e8-857d-1173a9da955c.png" align="left" width="32%" />
<img src="https://user-images.githubusercontent.com/40266/47967520-691f3280-e05e-11e8-838a-83706df2edb0.png" align="left" width="22%" />

On a Wemos D1 mini:

|Wemos GPIO |NRF24  |Color |
|-----------|-------|------|
|GND        |GND    |Black |
|3V3        |VCC    |Red   |
|D2 GPIO4   |CE     |Orange|
|D5 GPIO14  |SCK    |Green |
|D6 GPIO12  |MISO   |Violet|
|D7 GPIO13  |MOSI   |Blue  |
|D8 GPIO15  |CSN    |Yellow|

* Do not mount the NRF24 and ESP12 antennas against each other. This will cause bad performance and crashes

_Image source: [MySensors.org](https://mysensors.org)_

##### LT8900

Connect SPI pins (CE, SCK, MOSI, MISO) to appropriate SPI pins on the ESP8266. With default settings, connect RST to GPIO 0, PKT to GPIO 16, CE to GPIO 4, and CSN to GPIO 15.  Make sure to properly configure these if using non-default pinouts.

#### Setting up the ESP

The goal here is to flash your ESP with the firmware. This should be done with ESPHome.
* Create a yaml file with the example provided.
* Import the "milight" directory by copying the files to the root directory of your ESPHome installation. (In my case the root directory is "\config\esphome\")
* Deploy this to your devices. 

#### Pair Bulbs

If you need to pair some bulbs, how to do this is [described in the wiki](https://github.com/sidoh/esp8266_milight_hub/wiki/Pairing-new-bulbs).
* When the ESPHome module is deployed in "DEBUG" mode, it will log received and decoded packets to determine Device and Group id's from your existing remotes

# 2. DCF77 module ESPhome
See the attached example "example_dcf77.yaml"
