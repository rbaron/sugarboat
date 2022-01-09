#ifndef _SUGARBOAT_BLE_H_
#define _SUGARBOAT_BLE_H_

#include <bluefruit.h>

namespace sugarboat {
namespace {
constexpr char kConfigServiceUUID[] = "e94989ee-7b22-4b34-b71d-a33459aea9ae";
constexpr char kConfigCharacteristicUUID[] =
    "e65785ce-955b-42aa-95a2-b7d96806d3da";
}  // namespace

class BLE {
 public:
  BLE()
      : cfg_service_(kConfigServiceUUID),
        cfg_char_(kConfigCharacteristicUUID) {}

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
  BLEService cfg_service_;
  BLECharacteristic cfg_char_;
};

}  // namespace sugarboat

#endif  // _SUGARBOAT_BLE_H_