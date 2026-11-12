#pragma once
#include <stdint.h>

namespace Py32Updater
{
    // Call after Wire.begin(), with exclusive access to the I2C bus.
    // Pass a free GPIO wired to NRST, or omit it for manual reset.
    bool update(int resetPin = -1);
    // Read the NoiseModuleMVP protocol version at 0x42 without writing Flash.
    bool checkApplication(uint32_t timeout = 5000);
}
