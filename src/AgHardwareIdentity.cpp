#include "AgHardwareIdentity.h"

#if defined(ESP32)
#include "sdkconfig.h"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3)
#include "esp_efuse.h"
#endif

namespace {

constexpr uint8_t HARDWARE_IDENTITY_UNPROVISIONED = 0x00;
constexpr uint8_t HARDWARE_IDENTITY_INDOOR = 0xA5;
constexpr uint8_t HARDWARE_IDENTITY_OUTDOOR = 0x5A;

} // namespace

void AgHardwareIdentity::begin(void) {
  _value = Value::Unprovisioned;
  _rawValue = HARDWARE_IDENTITY_UNPROVISIONED;

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  uint8_t block[32] = {};
  if (esp_efuse_read_block(EFUSE_BLK_USER_DATA, block, 0, sizeof(block) * 8) !=
      ESP_OK) {
    _value = Value::Invalid;
    return;
  }
  _rawValue = block[0];

  switch (_rawValue) {
  case HARDWARE_IDENTITY_UNPROVISIONED:
    _value = Value::Unprovisioned;
    break;
  case HARDWARE_IDENTITY_INDOOR:
    _value = Value::Indoor;
    break;
  case HARDWARE_IDENTITY_OUTDOOR:
    _value = Value::Outdoor;
    break;
  default:
    _value = Value::Invalid;
    break;
  }
#endif
}

BoardType AgHardwareIdentity::resolve(bool oledDetected,
                                      const String &model) const {
  if (_value == Value::Indoor) {
    return BoardType::ONE_INDOOR;
  }
  if (_value == Value::Outdoor) {
    return BoardType::OPEN_AIR_OUTDOOR;
  }
  if (oledDetected || model.startsWith("I-")) {
    return BoardType::ONE_INDOOR;
  }
  return BoardType::OPEN_AIR_OUTDOOR;
}

bool AgHardwareIdentity::isProvisioned(void) const {
  return _value == Value::Indoor || _value == Value::Outdoor;
}

uint8_t AgHardwareIdentity::getRawValue(void) const { return _rawValue; }

const char *AgHardwareIdentity::getValueName(void) const {
  switch (_value) {
  case Value::Unprovisioned:
    return "unprovisioned";
  case Value::Indoor:
    return "indoor";
  case Value::Outdoor:
    return "outdoor";
  case Value::Invalid:
    return "invalid";
  }

  return "unknown";
}
