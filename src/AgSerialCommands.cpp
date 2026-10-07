#ifdef ESP32

#include "AgSerialCommands.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
bool parseNumber(const char *text, float &value) {
  // strtof also accepts hex and non-finite values; allow decimal tokens only.
  if (*text == '\0' || strspn(text, "0123456789+-.eE") != strlen(text)) {
    return false;
  }
  errno = 0;
  char *end;
  value = strtof(text, &end);
  return end != text && *end == '\0' && errno != ERANGE && std::isfinite(value);
}
} // namespace

AgSerialCommands::AgSerialCommands(Stream &serial, Handler handler)
    : serial(serial), handler(handler) {}

const char *AgSerialCommands::targetName(Target target) {
  switch (target) {
  case Target::PM:
    return "PM";
  case Target::Temperature:
    return "TEMP";
  case Target::Humidity:
    return "HUM";
  }
  return "";
}

void AgSerialCommands::reply(const char *body) {
  char output[272];
  // Start a new line even if the preceding firmware log was unterminated.
  const int count = snprintf(output, sizeof(output), "\n#AG %s\n", body);
  if (count > 0 && static_cast<size_t>(count) < sizeof(output)) {
    // One write keeps concurrent log writes out of the protocol line.
    serial.write(reinterpret_cast<const uint8_t *>(output), count);
  }
}

void AgSerialCommands::readInput() {
  // Bound each pass so a continuous input stream cannot starve the firmware.
  int remaining = serial.available();
  if (remaining > 512) {
    remaining = 512;
  }
  while (remaining-- > 0) {
    const int byte = serial.read();
    if (byte < 0) {
      break;
    }
    if (busyBytes > 0) {
      --busyBytes;
      lineBusy = true;
    }
    receive(static_cast<char>(byte));
  }
}

void AgSerialCommands::run() {
  readInput();
  if (!pending) {
    return;
  }
  char response[256] = "ERROR OPERATION_FAILED";
  handler(request, response, sizeof(response));
  // Requests already received while this operation ran are rejected as BUSY.
  readInput();
  // Carry the busy state across bounded passes when more input is queued.
  busyBytes = serial.available();
  reply(response);
  pending = false;
}

void AgSerialCommands::receive(char byte) {
  if (byte == '\n') {
    if (!discarding) {
      if (length > 0 && line[length - 1] == '\r') {
        --length;
      }
      line[length] = '\0';
      parseLine();
    }
    length = 0;
    discarding = false;
    lineBusy = false;
    return;
  }
  if (discarding) {
    return;
  }
  lineBusy = lineBusy || pending;
  if (length == MaxLineLength) {
    discarding = true;
    return;
  }
  line[length++] = byte;
}

void AgSerialCommands::parseLine() {
  if (length < 4 || memcmp(line, "#AG ", 4) != 0) {
    return;
  }
  for (size_t i = 4; i < length; ++i) {
    if ((line[i] < ' ' && line[i] != '\t') || line[i] > '~') {
      reply("ERROR INVALID_ARGUMENT");
      return;
    }
  }
  char *tokens[5] = {};
  size_t count = 0;
  char *save = nullptr;
  for (char *token = strtok_r(line + 4, " \t", &save); token != nullptr;
       token = strtok_r(nullptr, " \t", &save)) {
    if (count == 5) {
      reply("ERROR INVALID_ARGUMENT");
      return;
    }
    tokens[count++] = token;
  }
  if (count == 0) {
    reply("ERROR EMPTY_COMMAND");
    return;
  }
  Request next = {};
  size_t expected = 1;
  if (strcmp(tokens[0], "HELP") == 0) {
    next.command = Command::Help;
  } else if (strcmp(tokens[0], "GET_SERIAL") == 0) {
    next.command = Command::GetSerial;
  } else if (strcmp(tokens[0], "FACTORY_RESET") == 0) {
    next.command = Command::FactoryReset;
  } else if (strcmp(tokens[0], "SET_SLR") == 0) {
    next.command = Command::SetSlr;
    expected = 4;
  } else if (strcmp(tokens[0], "GET_SLR") == 0) {
    next.command = Command::GetSlr;
    expected = 2;
  } else {
    reply("ERROR INVALID_COMMAND");
    return;
  }
  if (count != expected) {
    reply("ERROR INVALID_ARGUMENT");
    return;
  }
  if (expected > 1) {
    if (strcmp(tokens[1], "PM") == 0) {
      next.target = Target::PM;
    } else if (strcmp(tokens[1], "TEMP") == 0) {
      next.target = Target::Temperature;
    } else if (strcmp(tokens[1], "HUM") == 0) {
      next.target = Target::Humidity;
    } else {
      reply("ERROR INVALID_ARGUMENT");
      return;
    }
  }
  if (expected == 4 &&
      (!parseNumber(tokens[2], next.scale) || !parseNumber(tokens[3], next.intercept))) {
    reply("ERROR INVALID_ARGUMENT");
    return;
  }
  if (pending || lineBusy) {
    reply("ERROR BUSY");
    return;
  }
  request = next;
  pending = true;
}

#endif // ESP32
