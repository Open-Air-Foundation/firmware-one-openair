#ifndef AG_HARDWARE_IDENTITY_H
#define AG_HARDWARE_IDENTITY_H

#include "Main/BoardDef.h"
#include <Arduino.h>
#include <stdint.h>

class AgHardwareIdentity {
public:
  void begin(void);
  BoardType resolve(bool oledDetected, const String &model) const;

  bool isProvisioned(void) const;
  uint8_t getRawValue(void) const;
  const char *getValueName(void) const;

private:
  enum class Value {
    Unprovisioned,
    Indoor,
    Outdoor,
    Invalid,
  };

  Value _value = Value::Unprovisioned;
  uint8_t _rawValue = 0;
};

#endif /** AG_HARDWARE_IDENTITY_H */
