#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>

namespace esphome {
namespace canbus_gvret {

class WiFiManager
{
public:
    WiFiManager();
    void setup();
    void loop();
    void sendBufferedData();
    
private:
    WiFiServer wifiServer;
    WiFiClient wifiClient;
    WiFiUDP wifiUDPServer;
    uint32_t lastBroadcast;
};

}
}
