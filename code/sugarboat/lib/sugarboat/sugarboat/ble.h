#ifndef _SUGARBOAT_BLE_H_
#define _SUGARBOAT_BLE_H_

#include <bluefruit.h>

namespace sugarboat {
class BLE {
 public:
  BLE() {}

  // Disable copying.
  BLE(const BLE&) = delete;
  BLE& operator=(const BLE&) = delete;

  bool Init();
  bool StartAdv();

  bool IsConnected() {
    return Bluefruit.connected();
  }

  // Not ideal to return an internal handle but let's keep it simple.
  BLEUart& GetBLEUart() {
    return bleuart_;
  }

 private:
  BLEUart bleuart_;
};

}  // namespace sugarboat

#endif  // _SUGARBOAT_BLE_H_