#ifndef _SUGARBOAT_BLE_H_
#define _SUGARBOAT_BLE_H_

#include <bluefruit.h>

#include "sugarboat/config.h"
#include "sugarboat/mpu6050.h"
#include "sugarboat/sensor_data.h"

namespace sugarboat {
namespace {

constexpr char kSensorServiceUUID[] = "9b7d5c6f-a8ca-4080-9290-d4afb5ac64a3";
constexpr char kSensorCharacteristicUUID[] =
    "527d0f9b-db66-48c5-9089-071e1a795b6f";
constexpr char kConfigServiceUUID[] = "e94989ee-7b22-4b34-b71d-a33459aea9ae";
constexpr char kConfigCharacteristicUUID[] =
    "e65785ce-955b-42aa-95a2-b7d96806d3da";
}  // namespace

class BLE {
 public:
  static BLE& GetInstance() {
    static BLE ble;
    return ble;
  }

  // Disable copying.
  BLE(const BLE&) = delete;
  BLE& operator=(const BLE&) = delete;

  bool Init(Config& config, IMU& imu);
  bool StartAdv();

  bool IsConnected() {
    return Bluefruit.connected();
  }

  // Not ideal to return an internal handle but let's keep it simple.
  BLEUart& GetBLEUart() {
    return bleuart_;
  }

  bool InjectSensorData(const SensorData& sensor_data);

 private:
  BLE()
      : cfg_service_(kConfigServiceUUID),
        cfg_char_(kConfigCharacteristicUUID),
        sensor_service_(kSensorServiceUUID),
        sensor_char_(kSensorCharacteristicUUID) {}

  static void CfgCharWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr,
                                   uint8_t* data, uint16_t len);

  static void ConnCallback(uint16_t conn_handle);

  static void DisconnCallback(uint16_t conn_handle, uint8_t reason);

  int n_conns_ = 0;
  IMU* imu_ = nullptr;
  Config* config_ = nullptr;
  BLEUart bleuart_;
  BLEService cfg_service_;
  BLECharacteristic cfg_char_;
  BLEService sensor_service_;
  BLECharacteristic sensor_char_;
};

}  // namespace sugarboat

#endif  // _SUGARBOAT_BLE_H_