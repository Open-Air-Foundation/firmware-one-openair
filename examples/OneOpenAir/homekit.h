#ifndef ONEOPENAIR_HOMEKIT_H
#define ONEOPENAIR_HOMEKIT_H

// Bridge between the OneOpenAir sketch and Apple HomeKit (HomeSpan).
//
// The implementation lives in homekit.cpp, which includes HomeSpan.h but never
// AirGradient.h. This keeps HomeSpan's global `PushButton` class out of the same
// translation unit as the AirGradient library's class of the same name, which
// would otherwise collide. This header therefore exposes only plain types.

// Build the HomeKit accessory and adopt the WiFi connection the firmware has
// already established (HomeSpan reuses it rather than managing WiFi itself).
// Call once, after WiFi is connected. All strings are copied by HomeSpan.
void homekitBegin(const char *model, const char *serialNumber,
                  const char *firmwareVersion, const char *ssid, const char *psk);

// Service HomeKit. Call frequently (every loop iteration).
void homekitPoll();

// True once homekitBegin() has built the accessory.
bool homekitIsReady();

// Forward the latest readings to HomeKit. Each *Valid flag indicates whether the
// paired reading is currently valid; invalid readings are not forwarded, leaving
// the previous HomeKit value intact.
void homekitUpdate(float pm25, bool pm25Valid, int co2, bool co2Valid, int tvoc,
                   bool tvocValid, int nox, bool noxValid, float temperature,
                   bool temperatureValid, float humidity, bool humidityValid);

#endif // ONEOPENAIR_HOMEKIT_H
