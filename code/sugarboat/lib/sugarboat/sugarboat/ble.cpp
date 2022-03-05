#include "sugarboat/ble.h"

#include <string>

#include "sugarboat/sensor_data.h"

namespace sugarboat {
namespace {
constexpr int kMaxConnections = 2;
constexpr int kSensorCharProtocolVersion = 0;

class StringStream : public Stream {
 public:
  int available() override {
    return ptr - str_.size();
  }
  int read() {
    return str_[ptr++];
  }
  int peek() {
    return str_[ptr];
  }
  void flush() {}

  size_t write(uint8_t byte) override {
    str_ += byte;
    return 1;
  }

  std::string& GetString() {
    return str_;
  }

 private:
  std::string str_;
  size_t ptr = 0;
};

bool WriteToConfigChar(const Config& cfg, BLECharacteristic& chr) {
  StringStream config_stream;
  if (cfg.Serialize(config_stream) <= 0) {
    Serial.println("[ble] Wrote less than or equal 0 bytes of config\n");
    return false;
  }

  std::string& data = config_stream.GetString();
  size_t char_written = chr.write(data.c_str(), data.size());
  if (char_written < data.size()) {
    Serial.printf("[ble] Wrote %d bytes to config char and expected %d\n",
                  char_written, data.size());
    return false;
  }

  Serial.printf("[ble] Wrote cfg to ble char: %s\n", data.c_str());

  // Maybe notify clients.
  for (int conn_handler = 0; conn_handler < kMaxConnections; conn_handler++) {
    if (Bluefruit.connected(conn_handler) && chr.notifyEnabled(conn_handler)) {
      chr.notify(conn_handler, data.c_str(), data.size());
    }
  }
  return true;
}

}  // namespace

bool BLE::Init(Config& config, IMU& imu) {
  config_ = &config;
  imu_ = &imu;

  Bluefruit.autoConnLed(false);

  if (!Bluefruit.begin(kMaxConnections)) {
    Serial.println("[ble] Error initializing Bluefruit");
    return false;
  }

  Bluefruit.setTxPower(0);

  Bluefruit.setName(config.GetName().c_str());
  Bluefruit.Periph.setConnectCallback(ConnCallback);
  Bluefruit.Periph.setDisconnectCallback(DisconnCallback);
  Bluefruit.Periph.setConnInterval(800, 1600);

  // if (bleuart_.begin()) {
  //   Serial.println("[ble] Error initializing BLE UART service");
  //   return false;
  // }

  // Init config service & characteristic.
  if (cfg_service_.begin()) {
    Serial.println("[ble] Error initializing BLE config service");
    return false;
  }
  cfg_char_.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY | CHR_PROPS_WRITE);
  cfg_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  cfg_char_.setWriteCallback(CfgCharWriteCallback);
  cfg_char_.setMaxLen(512);
  if (cfg_char_.begin()) {
    Serial.println("[ble] Error initializing BLE config characteristic");
    return false;
  }
  WriteToConfigChar(*config_, cfg_char_);

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

  ble_gap_addr_t addr = Bluefruit.getAddr();
  Serial.printf("[ble] MAC Address: ");
  Serial.printBufferReverse(addr.addr, BLE_GAP_ADDR_LEN, ':');
  Serial.println();

  return true;
}

bool BLE::StartAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  Bluefruit.Advertising.addService(sensor_service_);
  Bluefruit.Advertising.addService(bleuart_);
  Bluefruit.Advertising.addService(cfg_service_);

  Bluefruit.ScanResponse.addName();

  // Bluefruit.Advertising.restartOnDisconnect(true);
  // Advertisement interval in units of 625 us. For estimating impact in battery
  // life, use
  // https://devzone.nordicsemi.com/nordic/power/w/opp/2/online-power-profiler-for-ble.
  // Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setInterval(32, 3200);
  Bluefruit.Advertising.setFastTimeout(1);
  Bluefruit.Advertising.start(0);

  return true;
}

template <typename Type>
inline static void Encode16BitFloat(float val, uint8_t* buf, size_t idx,
                                    int scale) {
  Type value = scale * val;
  memcpy(buf + idx, &value, 2);
}

bool BLE::InjectSensorData(const SensorData& sensor_data) {
  uint8_t buf[14];

  // Byte 0
  //   Bits 0-3: Protocol version.
  buf[0] = kSensorCharProtocolVersion & 0x7;
  // Byte 1 is reversed for future use.
  buf[1] = 0x00;

  // Bytes 2 - 3: angle in degrees * 10.
  Encode16BitFloat<int16_t>(sensor_data.tilt_degrees, buf, 2, 10);

  // Bytes 4 - 5: grams of sugar in 100 grams of solution (Brix scale).
  float brix = CalculateBrix(config_->GetCoeffs(), sensor_data.tilt_degrees);
  Encode16BitFloat<uint16_t>(brix, buf, 4, 100);

  // Bytes 6 - 7: specific gravity ("SG" scale).
  Encode16BitFloat<uint16_t>(BrixToSG(brix), buf, 6, 1000);

  // Bytes 8 - 9: temp in Celcius * 100.
  Encode16BitFloat<int16_t>(sensor_data.temp_celcius, buf, 8, 100);

  // Bytes 10 - 11: relative humidity in range [0, UINT16_MAX].
  Encode16BitFloat<uint16_t>(sensor_data.rel_humi / 100, buf, 10, UINT16_MAX);

  // Bytes 12 - 13: batt voltage * 1000.
  Encode16BitFloat<uint16_t>(sensor_data.batt_volt, buf, 12, 1000);

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
      sensor_char_.notify(conn_handler, buf, sizeof(buf));
    }
  }
  return true;
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

  if (len < 1) {
    Serial.printf("[ble] Insufficient data: %d bytes\n", len);
  }
  switch (data[0]) {
    case 0x01: {
      Serial.printf("[ble] Will calibrate IMU\n");
      suspendLoop();
      IMU::Offsets offsets = ble.imu_->Calibrate();
      ble.config_->SetIMUOffsets(offsets);
      ble.config_->CommitToFlash();
      resumeLoop();
      return;
    }
    case 0x02: {
      Serial.printf("[ble] Will set polynomial coefficients\n");
      Coeffs coeffs;
      // Nasty error prone memcpy. It expects 3 32-bit IEEE-754 in
      // little-endian.
      memcpy(&coeffs, data + 1, sizeof(Coeffs));
      Serial.printf("[ble] Coeffs: %f, %f, %f\n", coeffs.a2, coeffs.a1,
                    coeffs.a0);
      ble.config_->SetCoeffs(coeffs);
      suspendLoop();
      ble.config_->CommitToFlash();
      resumeLoop();
      return;
    }
    case 0x03: {
      Serial.printf("[ble] Will set realtime run %d\n", data[1]);
      ble.config_->SetRealtimeRun(data[1] != 0);
      return;
    }
    case 0x04: {
      std::string name((char*)data + 1, len - 1);
      Serial.printf("[ble] Will set name to: %s\n", name.c_str());
      ble.config_->SetName(name);
      suspendLoop();
      ble.config_->CommitToFlash();
      resumeLoop();
      return;
    }
    default:
      Serial.printf("[ble] Unhandled config action: 0x%02x\n", data[0]);
      return;
  }
}

void BLE::ConnCallback(uint16_t conn_handle) {
  BLE& ble = BLE::GetInstance();
  digitalToggle(LED_BUILTIN);
  if (++ble.n_conns_ < kMaxConnections) {
    Bluefruit.Advertising.start(0);
  }

  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  uint16_t conn_int = conn->getConnectionInterval();
  Serial.printf("[ble] Connection interval: %d\n", conn_int);

  // Request the connecting central to change the connection.
  // Note that this also dramatically increases the time it takes to stablish a
  // connection. This is surprising because I assumed this callback would only
  // be called _after_ the connection is fully stablished.
  conn->requestConnectionParameter(300);

  Serial.printf("[ble] Connection callback. #clients: %d\n", ble.n_conns_);
}

void BLE::DisconnCallback(uint16_t conn_handle, uint8_t reason) {
  BLE& ble = BLE::GetInstance();
  --ble.n_conns_;
  Serial.printf("[ble] Disconnect callback. #clients: %d\n", ble.n_conns_);
  ble.config_->SetRealtimeRun(false);

  // Keep or resume advertising.
  Bluefruit.Advertising.start(0);
}

}  // namespace sugarboat