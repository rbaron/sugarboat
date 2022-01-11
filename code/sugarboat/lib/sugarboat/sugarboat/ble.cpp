#include "sugarboat/ble.h"

#include "sugarboat/sensor_data.h"

namespace sugarboat {
namespace {
constexpr int kMaxConnections = 2;
constexpr int kSensorCharProtocolVersion = 0;

bool EncodeSensorData(SensorData& sensor_data, uint8_t* buf, size_t bufsize) {
  if (bufsize < 8) {
    return false;
  }

  // Byte 0
  //   Bits 0-3: Protocol version.
  buf[0] = kSensorCharProtocolVersion & 0x7;
  // Byte 1 is reversed for future use.
  buf[1] = 0x00;

  // Bytes 2 - 3: angle in milli radians.
  buf[2] = sensor_data.angle_mrad >> 8;
  buf[3] = sensor_data.angle_mrad & 0xff;

  // Bytes 4 - 5: temp in millidegrees Celcius.
  buf[4] = sensor_data.temp_mcelcius >> 8;
  buf[5] = sensor_data.temp_mcelcius & 0xff;

  // Bytes 6 - 7: relative humidity in range [0, UINT16_MAX].
  buf[6] = sensor_data.humi >> 8;
  buf[7] = sensor_data.humi & 0xff;

  return true;
}
}  // namespace

bool BLE::Init(Config& config) {
  config_ = &config;

  Bluefruit.autoConnLed(false);

  if (!Bluefruit.begin(kMaxConnections)) {
    Serial.println("[ble] Error initializing Bluefruit");
    return false;
  }

  Bluefruit.setTxPower(4);
  Bluefruit.setName("sugarboat");
  Bluefruit.Periph.setConnectCallback(ConnCallback);
  Bluefruit.Periph.setDisconnectCallback(DisconnCallback);

  if (bleuart_.begin()) {
    Serial.println("[ble] Error initializing BLE UART service");
    return false;
  }

  // Init config service & characteristic.
  if (cfg_service_.begin()) {
    Serial.println("[ble] Error initializing BLE config service");
    return false;
  }
  cfg_char_.setProperties(CHR_PROPS_WRITE);
  cfg_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  cfg_char_.setWriteCallback(CfgCharWriteCallback);
  if (cfg_char_.begin()) {
    Serial.println("[ble] Error initializing BLE config characteristic");
    return false;
  }

  // Init sensor service & characteristic.
  if (sensor_service_.begin()) {
    Serial.println("[ble] Error initializing BLE sensor service");
    return false;
  }
  sensor_char_.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  sensor_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  if (sensor_char_.begin()) {
    Serial.println("[ble] Error initializing BLE sensor characteristic");
    return false;
  }
  orientation_char_.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  orientation_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  if (orientation_char_.begin()) {
    Serial.println("[ble] Error initializing BLE orientation characteristic");
    return false;
  }

  return true;
}

bool BLE::StartAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  Bluefruit.Advertising.addService(bleuart_);
  Bluefruit.Advertising.addService(cfg_service_);
  Bluefruit.Advertising.addService(sensor_service_);

  Bluefruit.ScanResponse.addName();

  // Bluefruit.Advertising.restartOnDisconnect(true);
  // Advertisement interval in units of 625 us. For estimating impact in battery
  // life, use
  // https://devzone.nordicsemi.com/nordic/power/w/opp/2/online-power-profiler-for-ble.
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);

  return true;
}

bool BLE::InjectSensorData(const SensorData& sensor_data) {
  uint8_t buf[8];

  // Byte 0
  //   Bits 0-3: Protocol version.
  buf[0] = kSensorCharProtocolVersion & 0x7;
  // Byte 1 is reversed for future use.
  buf[1] = 0x00;

  // Bytes 2 - 3: angle in milli radians.
  buf[2] = sensor_data.angle_mrad >> 8;
  buf[3] = sensor_data.angle_mrad & 0xff;

  // Bytes 4 - 5: specific gravity ("SG" scale).

  // Bytes 5 - 6: grams of sugar in 100 grams of solution (Brix scale).

  // Bytes 4 - 5: temp in millidegrees Celcius.
  buf[4] = sensor_data.temp_mcelcius >> 8;
  buf[5] = sensor_data.temp_mcelcius & 0xff;

  // Bytes 6 - 7: relative humidity in range [0, UINT16_MAX].
  buf[6] = sensor_data.humi >> 8;
  buf[7] = sensor_data.humi & 0xff;

  uint16_t written_len = sensor_char_.write(buf, sizeof(buf));
  if (written_len < sizeof(buf)) {
    Serial.printf(
        "[ble] Did not write enough bytes to sensor characteristic\n");
    return false;
  }

  // Maybe notify clients.
  for (int conn_handler = 0; conn_handler < kMaxConnections; conn_handler++) {
    if (Bluefruit.connected(conn_handler) &&
        sensor_char_.notifyEnabled(conn_handler)) {
      sensor_char_.notify(buf, sizeof(buf));
    }
  }
}

inline static void Encode16BitFloat(float val, uint8_t* buf, size_t idx) {
  int16_t value = 1000 * val;
  memcpy(buf + idx, &value, 2);
}

bool BLE::InjectOrientationData(const IMU::Orientation& orientation) {
  uint8_t buf[8];
  Encode16BitFloat(orientation.quaternion.w, buf, 0);
  Encode16BitFloat(orientation.quaternion.x, buf, 2);
  Encode16BitFloat(orientation.quaternion.y, buf, 4);
  Encode16BitFloat(orientation.quaternion.z, buf, 6);

  uint16_t written_len = orientation_char_.write(buf, sizeof(buf));
  if (written_len < sizeof(buf)) {
    Serial.printf(
        "[ble] Did not write enough bytes to orientation characteristic\n");
    return false;
  }

  // Maybe notify clients.
  for (int conn_handler = 0; conn_handler < kMaxConnections; conn_handler++) {
    if (Bluefruit.connected(conn_handler) &&
        orientation_char_.notifyEnabled(conn_handler)) {
      orientation_char_.notify(buf, sizeof(buf));
    }
  }
}

void BLE::CfgCharWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr,
                               uint8_t* data, uint16_t len) {
  BLE& ble = BLE::GetInstance();
  if (ble.config_ == nullptr) {
    Serial.printf(
        "[ble] Error in config write callback - BLE has not been initialized");
    return;
  }
  Serial.printf("[ble] Private CFG write callback len %d\n", len);
}

void BLE::ConnCallback(uint16_t conn_handle) {
  BLE& ble = BLE::GetInstance();
  digitalToggle(LED_BUILTIN);
  if (++ble.n_conns_ < kMaxConnections) {
    Bluefruit.Advertising.start(0);
  }
}

void BLE::DisconnCallback(uint16_t conn_handle, uint8_t reason) {
  BLE& ble = BLE::GetInstance();
  --ble.n_conns_;
}

}  // namespace sugarboat