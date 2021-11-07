#include <RF24.h>
#include "../Radio/PL1167_nRF24.h"
#include "../Radio/MiLightRadioConfig.h"
#include "../Radio/MiLightRadio.h"
#include "../Radio/NRF24MiLightRadio.h"
#include "../Radio/LT8900MiLightRadio.h"
#include "../Types/RF24PowerLevel.h"
#include "../Types/RF24Channel.h"
#include "../Settings/Settings.h"
#include <vector>
#include <memory>

#ifndef _MILIGHT_RADIO_FACTORY_H
#define _MILIGHT_RADIO_FACTORY_H

class MiLightRadioFactory {
public:

  virtual ~MiLightRadioFactory() { };
  virtual std::shared_ptr<MiLightRadio> create(const MiLightRadioConfig& config) = 0;

  static std::shared_ptr<MiLightRadioFactory> fromSettings(const Settings& settings);

};

class NRF24Factory : public MiLightRadioFactory {
public:

  NRF24Factory(
    uint8_t cePin,
    uint8_t csnPin,
    RF24PowerLevel rF24PowerLevel,
    const std::vector<RF24Channel>& channels,
    RF24Channel listenChannel
  );

  virtual std::shared_ptr<MiLightRadio> create(const MiLightRadioConfig& config);

protected:

  RF24 rf24;
  const std::vector<RF24Channel>& channels;
  const RF24Channel listenChannel;

};

class LT8900Factory : public MiLightRadioFactory {
public:

  LT8900Factory(uint8_t csPin, uint8_t resetPin, uint8_t pktFlag);

  virtual std::shared_ptr<MiLightRadio> create(const MiLightRadioConfig& config);

protected:

  uint8_t _csPin;
  uint8_t _resetPin;
  uint8_t _pktFlag;

};

#endif
