#pragma once

#include <Arduino.h>

enum MiLightRemoteType {
  REMOTE_TYPE_UNKNOWN = 255,
  REMOTE_TYPE_RGBW    = 0,
  REMOTE_TYPE_CCT     = 1,
  REMOTE_TYPE_RGB_CCT = 2,
  REMOTE_TYPE_RGB     = 3,
  REMOTE_TYPE_FUT089  = 4,
  REMOTE_TYPE_FUT091  = 5,
  REMOTE_TYPE_FUT020  = 6,
  REMOTE_TYPE_S2      = 7,
};

class MiLightRemoteTypeHelpers {
public:
  static const MiLightRemoteType remoteTypeFromString(const String& type);
  static const String remoteTypeToString(const MiLightRemoteType type);
  static const bool supportsRgb(const MiLightRemoteType type);
  static const bool supportsRgbw(const MiLightRemoteType type);
  static const bool supportsColorTemp(const MiLightRemoteType type);
};