#ifndef AG_SERIAL_COMMANDS_H
#define AG_SERIAL_COMMANDS_H

#ifdef ESP32

#include <Arduino.h>

class AgSerialCommands {
public:
  enum class Command { Help, GetSerial, SetSlr, GetSlr, FactoryReset };
  enum class Target { PM, Temperature, Humidity };
  struct Request {
    Command command;
    Target target;
    float scale;
    float intercept;
  };

  // Handler writes the response body ("OK ..." or "ERROR ...").
  using Handler = void (*)(const Request &request, char *response, size_t size);
  AgSerialCommands(Stream &serial, Handler handler);
  void run();
  static const char *targetName(Target target);

private:
  static constexpr size_t MaxLineLength = 128;
  Stream &serial;
  Handler handler;
  char line[MaxLineLength + 1] = {};
  size_t length = 0;
  bool discarding = false;
  bool lineBusy = false;
  bool pending = false;
  int busyBytes = 0;
  Request request = {};

  void readInput();
  void receive(char byte);
  void parseLine();
  void reply(const char *body);
};

#endif // ESP32
#endif // AG_SERIAL_COMMANDS_H
