#include "config.h"
#include "wifi_manager.h"
#include "gvret_comm.h"
#include <ESPmDNS.h>
#include <WiFi.h>

namespace esphome {
namespace canbus_gvret {

static IPAddress broadcastAddr(255,255,255,255);
boolean needServerInit = true; 

WiFiManager::WiFiManager()
{
    lastBroadcast = 0;
}

void WiFiManager::setup()
{
    //MDNS.addService("telnet", "tcp", 23);// Add service to MDNS-SD
    wifiServer.begin(23); //setup as a telnet server
    wifiServer.setNoDelay(true);
    Serial.println("");
    Serial.println(" ... ready!");
}

void WiFiManager::loop()
{ 
    int i;    

    if (wifiServer.hasClient())
    {
        for(i = 0; i < MAX_CLIENTS; i++)
        {
            if (!SysSettings.clientNodes[i] || !SysSettings.clientNodes[i].connected())
            {
                if (SysSettings.clientNodes[i]) SysSettings.clientNodes[i].stop();
                SysSettings.clientNodes[i] = wifiServer.available();
                if (!SysSettings.clientNodes[i]) Serial.println("Couldn't accept client connection!");
                else 
                {
                    Serial.print("New client: ");
                    Serial.print(i); Serial.print(' ');
                    Serial.println(SysSettings.clientNodes[i].remoteIP());            
                }
            }
        }
        if (i >= MAX_CLIENTS) {
            //no free/disconnected spot so reject
            wifiServer.available().stop();
        }
    }

    //check clients for data
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected())
        {
            if(SysSettings.clientNodes[i].available())
            {
                //get data from the telnet client and push it to input processing
                while(SysSettings.clientNodes[i].available()) 
                {
                    uint8_t inByt;
                    inByt = SysSettings.clientNodes[i].read();
                    wifiGVRET.processIncomingByte(inByt);
                }
            }
        }
        else
        {
            if (SysSettings.clientNodes[i]) 
            {
                SysSettings.clientNodes[i].stop();
            }
        }
    }                    


    if ((micros() - lastBroadcast) > 1000000ul) //every second send out a broadcast ping
    {
        uint8_t buff[4] = {0x1C,0xEF,0xAC,0xED};
        lastBroadcast = micros();
        wifiUDPServer.beginPacket(broadcastAddr, 17222);
        wifiUDPServer.write(buff, 4);
        wifiUDPServer.endPacket();
    }
}

void WiFiManager::sendBufferedData()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        size_t wifiLength = wifiGVRET.numAvailableBytes();
        uint8_t* buff = wifiGVRET.getBufferedBytes();
        if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected())
        {
            SysSettings.clientNodes[i].write(buff, wifiLength);
        }
    }
    wifiGVRET.clearBufferedBytes();
}
}
}
