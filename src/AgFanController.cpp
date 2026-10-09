#ifdef ESP32

#include "AgFanController.h"
#include <cmath>

FanController::FanController(TwoWire &wire)
    : emc230x(FAN_CONTROLLER_I2C_ADDRESS, wire), ready(false), enabled(false), minSpeed(0),
      maxSpeed(0), speedPercent(0), productId(0) {}

bool FanController::begin(bool enable, uint8_t minimumSpeed, uint8_t maximumSpeed) {
  ready = false;
  productId = 0;

  if (!setConfig(enable, minimumSpeed, maximumSpeed)) {
    return false;
  }

  if (!emc230x.beginWithoutWireInit()) {
    return false;
  }
  productId = emc230x.getProductID();

  if (!emc230x.setPWMFrequency(FAN_CONTROLLER_CHANNEL, PWM_FREQ_4882_HZ) ||
      !emc230x.setPWMOutputType(FAN_CONTROLLER_CHANNEL, true)) {
    return false;
  }

  FanConfig fanConfig{};
  fanConfig.enableClosedLoop = false;
  fanConfig.minRPM = 0;
  fanConfig.edges = 1;
  fanConfig.updateTime = 0;
  fanConfig.enableRampRate = false;
  fanConfig.enableGlitchFilter = true;
  fanConfig.errorWindow = 2;
  if (!emc230x.setFanConfig(FAN_CONTROLLER_CHANNEL, fanConfig)) {
    return false;
  }

  speedPercent = _calculateSpeedPercent(0, false, 0, false);
  if (!emc230x.setFanSpeedPercent(FAN_CONTROLLER_CHANNEL, speedPercent)) {
    return false;
  }

  ready = true;
  return true;
}

bool FanController::setConfig(bool enable, uint8_t minimumSpeed, uint8_t maximumSpeed) {
  if (minimumSpeed > maximumSpeed || maximumSpeed > 100) {
    return false;
  }
  enabled = enable;
  minSpeed = minimumSpeed;
  maxSpeed = maximumSpeed;
  return true;
}

bool FanController::update(float pm25Ugm3, bool hasPm25, float co2Ppm, bool hasCo2) {
  if (!ready) {
    return false;
  }

  const uint8_t speed = _calculateSpeedPercent(pm25Ugm3, hasPm25, co2Ppm, hasCo2);
  if (speed == speedPercent) {
    return true;
  }

  if (!emc230x.setFanSpeedPercent(FAN_CONTROLLER_CHANNEL, speed)) {
    return false;
  }

  speedPercent = speed;
  return true;
}

bool FanController::isReady(void) const { return ready; }

uint8_t FanController::getSpeedPercent(void) const { return speedPercent; }

int FanController::getActualRPM(void) {
  const uint16_t rpm = emc230x.getActualRPM(FAN_CONTROLLER_CHANNEL);
  if (rpm == 0) {
    return -1;
  }
  return rpm;
}

uint8_t FanController::getProductID(void) const { return productId; }

uint8_t FanController::_calculateSpeedPercent(float pm25Ugm3, bool hasPm25, float co2Ppm,
                                              bool hasCo2) const {
  if (!enabled) {
    return 0;
  }
  float speedBasedOnPm = FAN_CONTROLLER_DEFAULT_SPEED_PERCENT;
  if (hasPm25) {
    float pmRatio = 0.0f;
    if (pm25Ugm3 > 0.0f) {
      pmRatio = pm25Ugm3 / FAN_CONTROLLER_PM25_TARGET_UGM3;
      if (pmRatio > 1.0f) {
        pmRatio = 1.0f;
      }
    }
    speedBasedOnPm = minSpeed + ((maxSpeed - minSpeed) * pmRatio);
  }

  float speedBasedOnCo2 = 0.0f;
  if (hasCo2 && co2Ppm > FAN_CONTROLLER_CO2_PERFECT_PPM) {
    float co2Ratio = (co2Ppm - FAN_CONTROLLER_CO2_PERFECT_PPM) /
                     (FAN_CONTROLLER_CO2_TARGET_PPM - FAN_CONTROLLER_CO2_PERFECT_PPM);
    if (co2Ratio > 1.0f) {
      co2Ratio = 1.0f;
    }
    speedBasedOnCo2 = minSpeed + ((maxSpeed - minSpeed) * co2Ratio);
  }

  float speed = speedBasedOnPm;
  if (speedBasedOnCo2 > speed) {
    speed = speedBasedOnCo2;
  }
  if (!hasPm25 && !hasCo2) {
    speed = FAN_CONTROLLER_DEFAULT_SPEED_PERCENT;
  }

  if (speed < minSpeed) {
    speed = minSpeed;
  } else if (speed > maxSpeed) {
    speed = maxSpeed;
  }
  return static_cast<uint8_t>(std::round(speed));
}

#endif
