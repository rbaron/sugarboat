#include "sugarboat/logger.h"

namespace sugarboat {

int Logger::available() {
  return ble_.IsConnected() ? ble_.GetBLEUart().available()
                            : serial_.available();
};
int Logger::read() {
  return ble_.IsConnected() ? ble_.GetBLEUart().read() : serial_.read();
}
int Logger::peek() {
  return ble_.IsConnected() ? ble_.GetBLEUart().peek() : serial_.peek();
}
void Logger::flush() {
  ble_.IsConnected() ? ble_.GetBLEUart().flush() : serial_.flush();
}
size_t Logger::write(uint8_t byte) {
  return ble_.GetBLEUart().notifyEnabled() ? ble_.GetBLEUart().write(byte)
                                           : serial_.write(byte);
}
size_t Logger::write(const uint8_t* buffer, size_t size) {
  return ble_.GetBLEUart().notifyEnabled()
             ? ble_.GetBLEUart().write(buffer, size)
             : serial_.write(buffer, size);
}
}  // namespace sugarboat