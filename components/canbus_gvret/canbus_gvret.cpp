#include "esphome.h"
#include "canbus_gvret.h"
#include "config.h"
#include "wifi_manager.h"
#include "gvret_comm.h"

namespace esphome {
namespace canbus_gvret {

uint32_t lastFlushMicros = 0;

EEPROMSettings settings;
SystemSettings SysSettings;

WiFiManager wifiManager;

GVRET_Comm_Handler serialGVRET; //gvret protocol over the serial to USB connection
GVRET_Comm_Handler wifiGVRET; //GVRET over the wifi telnet port


void CanbusGVRET::setup() {

  ESP_LOGCONFIG(TAG, "Setting up CanbusGVRET");

  if (!this->setup_internal()) {
    ESP_LOGE(TAG, "setup error!");
    this->mark_failed();
  }

  wifiManager.setup();

  this->is_initialized = true;
  ESP_LOGCONFIG(TAG, "setup done");
}

float CanbusGVRET::get_setup_priority() const { return setup_priority::AFTER_WIFI; }

void CanbusGVRET::trigger(uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                                 const std::vector<uint8_t> &data) {
  // fire all triggers
  // TODO: currently we can't check can_id, can_mask, remote_transmission_request because these trigger fields
  // are protected
  for (auto *trigger : this->triggers_) {
    trigger->trigger(data, can_id, remote_transmission_request);
  }
}

bool CanbusGVRET::setup_internal() {

  if (!this->canbus) {
    return true;
  }
  
  Automation<std::vector<uint8_t>, uint32_t, bool> *automation;
  LambdaAction<std::vector<uint8_t>, uint32_t, bool> *lambdaaction;
  canbus::CanbusTrigger *canbus_canbustrigger;

  canbus_canbustrigger = new canbus::CanbusTrigger(this->canbus, 0, 0, false);
  canbus_canbustrigger->set_component_source("canbus_gvret");
  App.register_component(canbus_canbustrigger);
  automation = new Automation<std::vector<uint8_t>, uint32_t, bool>(canbus_canbustrigger);
  auto cb = [this](std::vector<uint8_t> x, uint32_t can_id, bool remote_transmission_request) -> void {
    trigger(can_id, this->use_extended_id_, remote_transmission_request, x);
    this->displayFrame(can_id, this->use_extended_id_, remote_transmission_request, x);
  };
  lambdaaction = new LambdaAction<std::vector<uint8_t>, uint32_t, bool>(cb);
  automation->add_actions({lambdaaction});
  ESP_LOGI(TAG, "Trigger installed!");

  return true;
}

canbus::Error CanbusGVRET::send_message(struct canbus::CanFrame *frame) {
  std::vector<uint8_t> data = std::vector<uint8_t>(frame->data, frame->data + frame->can_data_length_code);
  if (this->canbus) {
    this->canbus->send_data(frame->can_id, frame->use_extended_id, frame->remote_transmission_request, data);
  }
  this->displayFrame(frame->can_id, frame->use_extended_id, frame->remote_transmission_request, data);
  return canbus::ERROR_OK;
};

canbus::Error CanbusGVRET::read_message(struct canbus::CanFrame *frame) { return canbus::ERROR_NOMSG; };

void CanbusGVRET::displayFrame(uint32_t can_id, bool use_extended_id, bool remote_transmission_request, const std::vector<uint8_t> &data)
{
  if (!is_initialized)
  return;
  
  CAN_FRAME frame;

  frame.id = can_id;
  frame.extended = use_extended_id;
  frame.timestamp = millis();
  frame.length = data.size();
  
  for (int i = 0; i < data.size(); i++) {
    frame.data.uint8[i] = data[i];
  }

  wifiGVRET.sendFrameToBuffer(frame, 0);
  serialGVRET.sendFrameToBuffer(frame, 0);
}

void CanbusGVRET::loop() {

  if (!is_initialized)
  return;

  int serialCnt;
  uint8_t in_byte;

  wifiManager.loop();

  size_t wifiLength = wifiGVRET.numAvailableBytes();
  size_t serialLength = serialGVRET.numAvailableBytes();
  size_t maxLength = (wifiLength>serialLength) ? wifiLength : serialLength;

  //If the max time has passed or the buffer is almost filled then send buffered data out
  if ((micros() - lastFlushMicros > SER_BUFF_FLUSH_INTERVAL) || (maxLength > (WIFI_BUFF_SIZE - 40)) ) 
  {
      lastFlushMicros = micros();
      if (serialLength > 0) 
      {
          Serial.write(serialGVRET.getBufferedBytes(), serialLength);
          serialGVRET.clearBufferedBytes();
      }
      if (wifiLength > 0)
      {
          wifiManager.sendBufferedData();
      }
  }

  serialCnt = 0;
  while ( (Serial.available() > 0) && serialCnt < 128 ) 
  {
      serialCnt++;
      in_byte = Serial.read();
      serialGVRET.processIncomingByte(in_byte);
  }
}

}  // namespace canbus_gvret
}  // namespace esphome
