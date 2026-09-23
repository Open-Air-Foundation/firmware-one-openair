// Apple HomeKit support for the AirGradient ONE / Open Air, via HomeSpan.
//
// This file is deliberately isolated from the AirGradient library headers: it
// includes HomeSpan.h but not AirGradient.h. Both libraries define a global
// class named `PushButton`, so they cannot coexist in one translation unit.
// The sketch talks to this file only through the plain interface in homekit.h.

#include "homekit.h"

#include <Arduino.h>
#include <WiFi.h>

#include "HomeSpan.h"

namespace {

bool ready = false;

Characteristic::AirQuality *hkAirQuality = nullptr;
Characteristic::PM25Density *hkPm25 = nullptr;
Characteristic::VOCDensity *hkVoc = nullptr;
Characteristic::NitrogenDioxideDensity *hkNo2 = nullptr;
Characteristic::CarbonDioxideDetected *hkCo2Detected = nullptr;
Characteristic::CarbonDioxideLevel *hkCo2Level = nullptr;
Characteristic::CurrentTemperature *hkTemp = nullptr;
Characteristic::CurrentRelativeHumidity *hkHum = nullptr;

// PM2.5 sub-index, on HomeKit's AirQuality scale, adapted from the breakpoints
// used to control the LEDs (if PM2.5 is the chosen LED mode).
int pm25Index(float pm25) {
  if (pm25 <= 9) {
    return 1;
  } else if (pm25 <= 35) {
    return 2;
  } else if (pm25 <= 55) {
    return 3;
  } else if (pm25 <= 125) {
    return 4;
  }
  return 5;
}

// Logged on every HomeSpan status change (WiFi connecting, pairing needed,
// paired, etc.) — fires regardless of log level.
void hkStatusCallback(HS_STATUS status) {
  Serial.printf("[HomeKit] status: %s\n", homeSpan.statusString(status));
}

// Logged when a controller completes pairing or an existing pairing is removed.
void hkPairCallback(boolean isPaired) {
  Serial.printf("[HomeKit] %s\n", isPaired ? "PAIRED" : "unpaired / pairing lost");
}

// CO2 sub-index, on HomeKit's AirQuality scale, adapted from the breakpoints
// used to control the LEDs (if CO2 is the chosen LED mode).
int co2Index(int co2) {
  if (co2 <= 600) {
    return 1;
  } else if (co2 <= 1000) {
    return 2;
  } else if (co2 <= 2000) {
    return 3;
  } else if (co2 <= 3000) {
    return 4;
  }
  return 5;
}

} // namespace

void homekitBegin(const char *model, const char *serialNumber,
                  const char *firmwareVersion, const char *ssid, const char *psk) {
  // Adopt the firmware's existing WiFi connection instead of letting HomeSpan
  // manage WiFi: with credentials set and WiFi already up, HomeSpan's
  // checkConnect() reuses the connection rather than re-associating. Serial
  // input is disabled so HomeSpan's CLI does not consume the firmware's stream.
  homeSpan.setWifiCredentials(ssid, psk);
  homeSpan.setSerialInputDisable(true);
  homeSpan.setSketchVersion(firmwareVersion);

  // The AirGradient LocalServer already binds port 80, and HomeSpan's HAP server
  // defaults to 80 too. Without this, HomeSpan can't bind, so the accessory is
  // discoverable over mDNS but every pairing connection fails ("accessory not
  // reachable"). Move HAP to a free port.
  homeSpan.setPortNum(1201);

  // Diagnostics for pairing/sync. Log level 1 surfaces HomeSpan's own HAP and
  // pairing progress; the status/pair callbacks mark the key transitions.
  homeSpan.setLogLevel(1);
  homeSpan.setStatusCallback(hkStatusCallback);
  homeSpan.setPairCallback(hkPairCallback);

  homeSpan.begin(Category::Sensors, "AirGradient ONE", "airgradient", model);

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  new Characteristic::Manufacturer("AirGradient");
  new Characteristic::Model(model);
  new Characteristic::SerialNumber(serialNumber);
  new Characteristic::FirmwareRevision(firmwareVersion);
  new Characteristic::Name("AirGradient ONE");

  new Service::AirQualitySensor();
  new Characteristic::Name("Air Quality");
  hkAirQuality = new Characteristic::AirQuality(0);
  hkPm25 = new Characteristic::PM25Density(0);
  hkVoc = new Characteristic::VOCDensity(0);
  hkNo2 = new Characteristic::NitrogenDioxideDensity(0);

  new Service::CarbonDioxideSensor();
  new Characteristic::Name("CO2");
  hkCo2Detected = new Characteristic::CarbonDioxideDetected(0);
  hkCo2Level = new Characteristic::CarbonDioxideLevel(0);

  new Service::TemperatureSensor();
  new Characteristic::Name("Temperature");
  hkTemp = new Characteristic::CurrentTemperature(0);

  new Service::HumiditySensor();
  new Characteristic::Name("Humidity");
  hkHum = new Characteristic::CurrentRelativeHumidity(0);

  ready = true;
  Serial.println("HomeKit accessory ready");
}

void homekitPoll() {
  if (ready) {
    homeSpan.poll();
  }
}

bool homekitIsReady() { return ready; }

void homekitUpdate(float pm25, bool pm25Valid, int co2, bool co2Valid, int tvoc,
                   bool tvocValid, int nox, bool noxValid, float temperature,
                   bool temperatureValid, float humidity, bool humidityValid) {
  if (!ready) {
    return;
  }

  // Air quality index: worst (highest) of the available PM2.5 and CO2 sub-indices.
  int airQuality = 0;
  if (pm25Valid) {
    airQuality = max(airQuality, pm25Index(pm25));
    hkPm25->setVal(pm25);
  }
  if (co2Valid) {
    airQuality = max(airQuality, co2Index(co2));
    hkCo2Level->setVal(co2);
    hkCo2Detected->setVal(co2 > 1000 ? 1 : 0);
  }
  if (pm25Valid || co2Valid) {
    hkAirQuality->setVal(airQuality);
  }

  // TVOC/NOx are AirGradient index values (not ug/m3 densities), forwarded as-is.
  if (tvocValid) {
    hkVoc->setVal(tvoc);
  }
  if (noxValid) {
    hkNo2->setVal(nox);
  }
  if (temperatureValid) {
    hkTemp->setVal(temperature);
  }
  if (humidityValid) {
    hkHum->setVal(humidity);
  }
}
