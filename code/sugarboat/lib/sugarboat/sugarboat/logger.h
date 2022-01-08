#ifndef _SUGARBOAT_Logger_H_
#define _SUGARBOAT_Logger_H_

#include "sugarboat/ble.h"

namespace sugarboat {

class Logger : public Stream {
 public:
  Logger(Stream& serial, BLE& ble) : serial_(serial), ble_(ble) {}

  // Disable copying.
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  int available() override;
  int read() override;
  int peek() override;
  void flush() override;

  size_t write(uint8_t) override;
  size_t write(const uint8_t* buffer, size_t size) override;

 private:
  Stream& serial_;
  BLE& ble_;
};

}  // namespace sugarboat

#endif  // _SUGARBOAT_Logger_H_