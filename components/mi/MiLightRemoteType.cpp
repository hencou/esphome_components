#include "MiLightRemoteType.h"
#include "esphome/core/helpers.h"

static const char* REMOTE_NAME_RGBW    = "rgbw";
static const char* REMOTE_NAME_CCT     = "cct";
static const char* REMOTE_NAME_RGB_CCT = "rgb_cct";
static const char* REMOTE_NAME_FUT089  = "fut089";
static const char* REMOTE_NAME_RGB     = "rgb";
static const char* REMOTE_NAME_FUT091  = "fut091";
static const char* REMOTE_NAME_FUT020  = "fut020";
static const char* REMOTE_NAME_S2      = "s2";

const MiLightRemoteType MiLightRemoteTypeHelpers::remoteTypeFromString(const std::string& type) {
  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_RGBW) || esphome::str_equals_case_insensitive(type, "fut096")) {
    return REMOTE_TYPE_RGBW;
  }

  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_CCT) || esphome::str_equals_case_insensitive(type, "fut007")) {
    return REMOTE_TYPE_CCT;
  }

  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_RGB_CCT) || esphome::str_equals_case_insensitive(type, "fut092")) {
    return REMOTE_TYPE_RGB_CCT;
  }

  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_FUT089)) {
    return REMOTE_TYPE_FUT089;
  }

  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_RGB) || esphome::str_equals_case_insensitive(type, "fut098")) {
    return REMOTE_TYPE_RGB;
  }

  if (esphome::str_equals_case_insensitive(type, "v2_cct") || esphome::str_equals_case_insensitive(type, REMOTE_NAME_FUT091)) {
    return REMOTE_TYPE_FUT091;
  }

  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_FUT020)) {
    return REMOTE_TYPE_FUT020;
  }

  if (esphome::str_equals_case_insensitive(type, REMOTE_NAME_S2)) {
    return REMOTE_TYPE_S2;
  }

  //Serial.print(F("remoteTypeFromString: ERROR - tried to fetch remote config for type: "));
  //Serial.println(type);

  return REMOTE_TYPE_UNKNOWN;
}

const std::string MiLightRemoteTypeHelpers::remoteTypeToString(const MiLightRemoteType type) {
  switch (type) {
    case REMOTE_TYPE_RGBW:
      return REMOTE_NAME_RGBW;
    case REMOTE_TYPE_CCT:
      return REMOTE_NAME_CCT;
    case REMOTE_TYPE_RGB_CCT:
      return REMOTE_NAME_RGB_CCT;
    case REMOTE_TYPE_FUT089:
      return REMOTE_NAME_FUT089;
    case REMOTE_TYPE_RGB:
      return REMOTE_NAME_RGB;
    case REMOTE_TYPE_FUT091:
      return REMOTE_NAME_FUT091;
    case REMOTE_TYPE_FUT020:
      return REMOTE_NAME_FUT020;
    case REMOTE_TYPE_S2:
      return REMOTE_NAME_S2;
    default:
      //Serial.print(F("remoteTypeToString: ERROR - tried to fetch remote config name for unknown type: "));
      //Serial.println(type);
      return "unknown";
  }
}

const bool MiLightRemoteTypeHelpers::supportsRgbw(const MiLightRemoteType type) {
  switch (type) {
    case REMOTE_TYPE_FUT089:
    case REMOTE_TYPE_RGB_CCT:
    case REMOTE_TYPE_RGBW:
      return true;
    default:
      return false;
  }
}

const bool MiLightRemoteTypeHelpers::supportsRgb(const MiLightRemoteType type) {
  switch (type) {
    case REMOTE_TYPE_FUT089:
    case REMOTE_TYPE_RGB:
    case REMOTE_TYPE_RGB_CCT:
    case REMOTE_TYPE_RGBW:
    case REMOTE_TYPE_S2:
      return true;
    default:
      return false;
  }
}

const bool MiLightRemoteTypeHelpers::supportsColorTemp(const MiLightRemoteType type) {
  switch (type) {
    case REMOTE_TYPE_CCT:
    case REMOTE_TYPE_FUT089:
    case REMOTE_TYPE_FUT091:
    case REMOTE_TYPE_RGB_CCT:
    case REMOTE_TYPE_S2:
      return true;
    default:
      return false;
  }
}