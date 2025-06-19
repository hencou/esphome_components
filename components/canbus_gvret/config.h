
#ifndef CONFIG_H_
#define CONFIG_H_

#include <WiFi.h>

namespace esphome {
namespace canbus_gvret {

//size to use for buffering writes to USB. On the ESP32 we're actually talking TTL serial to a TTL<->USB chip
#define SER_BUFF_SIZE       1024

//Buffer for CAN frames when sending over wifi. This allows us to build up a multi-frame packet that goes
//over the air all at once. This is much more efficient than trying to send a new TCP/IP packet for each and every
//frame. It delays frames from getting to the other side a bit but that's life.
//Probably don't set this over 2048 as the default packet size for wifi is 2312 including all overhead.
#define WIFI_BUFF_SIZE      2048

//Number of microseconds between hard flushes of the serial buffer (if not in wifi mode) or the wifi buffer (if in wifi mode)
//This keeps the latency more consistent. Otherwise the buffer could partially fill and never send.
#define SER_BUFF_FLUSH_INTERVAL 20000

#define CFG_BUILD_NUM   618
#define MACC_NAME   "CAN"
#define NUM_BUSES   5   //max # of buses supported by any of the supported boards

#define SW_EN     2

//How many devices to allow to connect to our WiFi telnet port?
#define MAX_CLIENTS 1

struct FILTER {  //should be 10 bytes
    uint32_t id;
    uint32_t mask;
    boolean extended;
    boolean enabled;
} __attribute__((__packed__));

struct CANFDSettings {
    uint32_t nomSpeed;
    uint32_t fdSpeed;
    boolean enabled;
    boolean listenOnly;
    boolean fdMode;
};

struct EEPROMSettings {
    CANFDSettings canSettings[NUM_BUSES];

    boolean useBinarySerialComm; //use a binary protocol on the serial link or human readable format?
    uint8_t systemType; //0 = A0RET, 1 = EVTV ESP32 Board, 2 = Macchine 5-CAN board
} __attribute__((__packed__));

struct SystemSettings {

    WiFiClient clientNodes[MAX_CLIENTS];
};

class GVRET_Comm_Handler;

extern EEPROMSettings settings;
extern SystemSettings SysSettings;
extern GVRET_Comm_Handler serialGVRET;
extern GVRET_Comm_Handler wifiGVRET;

}
}

#endif /* CONFIG_H_ */
