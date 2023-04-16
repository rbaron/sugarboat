#include "sugarboat/ble.h"

#include <nrf_nvic.h>
#include <rtos.h>

#include <string>

#include "sugarboat/sensor_data.h"
#include "sugarboat/string_stream.h"

namespace sugarboat {
namespace {
constexpr int kMaxConnections = 2;
constexpr int kSensorCharProtocolVersion = 0;

// Time after which we'll switch to a slower advertising interval.
constexpr uint16_t kFastAdvIntervalSwitchSec = 120;
constexpr uint16_t kFastAdvIntervalMs = 320;
constexpr uint16_t kSlowAdvIntervalMs = 3200;

// Time after which we'll switch to a slower connection interval.
constexpr uint16_t kFastConnIntervalSwitchSec = 60;
constexpr uint16_t kSlowConnIntervalMs = 375;

struct client_ctx_t {
  uint16_t conn_handle;
  TimerHandle_t change_conn_interval_timer;
};

client_ctx_t client_ctx[kMaxConnections];

bool WriteToConfigChar(const Config& cfg, BLECharacteristic& chr) {
  // Serial.println("[ble config write] Raw config: ");
  // cfg.Serialize(Serial);
  // Serial.println();

  StringStream config_stream;
  if (cfg.Serialize(config_stream) <= 0) {
    Serial.println("[ble] Wrote less than or equal 0 bytes of config\n");
    return false;
  }

  const std::string& data = config_stream.GetString();
  Serial.printf("[ble write to config] Will write: %s\n", data.c_str());
  Serial.println();
  size_t char_written = chr.write(data.c_str(), data.size());
  if (char_written < data.size()) {
    Serial.printf("[ble] Wrote %d bytes to config char and expected %d\n",
                  char_written, data.size());
    return false;
  }

  // Serial.printf("[ble] Wrote cfg to ble char: %s\n", data.c_str());
  // Serial.println();
  Serial.printf("[ble] Wrote cfg to ble char\n");

  // This is broken. Despite us negotiating a larget MTU of 247, this
  // notification still chops up the data into 20 byte chunks. I think this is
  // because the BLE stack is still using the default MTU of 23.
  // // Maybe notify clients.
  // for (int conn_handler = 0; conn_handler < kMaxConnections; conn_handler++)
  // {
  //   if (Bluefruit.connected(conn_handler) && chr.notifyEnabled(conn_handler))
  //   {
  //     chr.notify(conn_handler, data.c_str(), data.size());
  //   }
  // }
  return true;
}

// Default: [20, 30] ms (actually maybe depend on runtime negotiation).
// Negotiated to 15ms: ~ 480 uA.
// 375 ms: 40 uA.
void UpdateConnInterval(uint16_t conn_handle, uint16_t interval_ms) {
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  uint16_t conn_int = conn->getConnectionInterval();
  Serial.printf(
      "[ble] Connection interval: %d ms. Requesting change to %d ms\n",
      MS125TO100(conn_int), interval_ms);

  // Request the connecting central to change the connection.
  // Note that this also dramatically increases the time it takes to stablish a
  // connection. This is surprising because I assumed this callback would only
  // be called _after_ the connection is fully established.
  bool res = conn->requestConnectionParameter(MS100TO125(interval_ms));
  Serial.printf("[ble] Requested connection interval change - success: %d\n",
                res);
}

}  // namespace

bool BLE::Init(Config& config, IMU& imu) {
  config_ = &config;
  imu_ = &imu;

  // Initialize client contexts.
  for (int i = 0; i < kMaxConnections; i++) {
    client_ctx[i].conn_handle = BLE_CONN_HANDLE_INVALID;
    client_ctx[i].change_conn_interval_timer = xTimerCreate(
        "change_conn_interval",
        pdMS_TO_TICKS(kFastConnIntervalSwitchSec * 1000),
        /*auto_reload=*/pdFALSE, (void*)i, [](TimerHandle_t timer) {
          // Race?
          int timer_id = (int)(pvTimerGetTimerID(timer));
          uint16_t conn_handle = client_ctx[timer_id].conn_handle;
          if (conn_handle == BLE_CONN_HANDLE_INVALID) {
            Serial.printf(
                "[ble] Timer callback for timer_id = %d got invalid "
                "conn_handle: %d\n",
                timer_id, conn_handle);
            return;
          }

          UpdateConnInterval(conn_handle, kSlowConnIntervalMs);
          Serial.printf(
              "[ble] Timer callback for timer_id = %d and conn_handle: %u - "
              "updated conn interval\n",
              timer_id, conn_handle);
        });
  }

  Bluefruit.autoConnLed(false);

  if (!Bluefruit.begin(kMaxConnections)) {
    Serial.println("[ble] Error initializing Bluefruit");
    return false;
  }

  Bluefruit.setTxPower(8);

  Bluefruit.setName(config.GetName().c_str());
  Bluefruit.Periph.setConnectCallback(ConnCallback);
  Bluefruit.Periph.setDisconnectCallback(DisconnCallback);
  Bluefruit.Periph.setConnInterval(800, 1600);

  // Init config service & characteristic.
  if (cfg_service_.begin()) {
    Serial.println("[ble] Error initializing BLE config service");
    return false;
  }
  cfg_char_.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  cfg_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  cfg_char_.setMaxLen(512);
  if (cfg_char_.begin()) {
    Serial.println("[ble] Error initializing BLE config characteristic");
    return false;
  }
  WriteToConfigChar(*config_, cfg_char_);

  cmd_char_.setProperties(CHR_PROPS_WRITE);
  cmd_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  cmd_char_.setWriteCallback(CmdCharWriteCallback);
  if (cmd_char_.begin()) {
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
  // @ 320, ~ 220 uA
  // @ 3200, ~ 40 uA
  Bluefruit.Advertising.setInterval(kFastAdvIntervalMs, kSlowAdvIntervalMs);
  Bluefruit.Advertising.setFastTimeout(kFastAdvIntervalSwitchSec);
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
  uint8_t buf[16];

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

  // Bytes 8 - 9: temp in Celsius * 100.
  Encode16BitFloat<int16_t>(sensor_data.temp_celsius, buf, 8, 100);

  // Bytes 10 - 11: relative humidity in range [0, UINT16_MAX].
  Encode16BitFloat<uint16_t>(sensor_data.rel_humi / 100, buf, 10, UINT16_MAX);

  // Bytes 12 - 13: batt voltage * 1000.
  Encode16BitFloat<uint16_t>(sensor_data.batt_volt, buf, 12, 1000);

  // Bytes 14 - 15: MPU6050's temp in Celsius * 100.
  Encode16BitFloat<int16_t>(sensor_data.mpu_temp_celsius, buf, 14, 100);

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

void BLE::CmdCharWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr,
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
      WriteToConfigChar(*ble.config_, ble.cfg_char_);
      return;
    }
    case 0x02: {
      Serial.printf("[ble] Will set polynomial coefficients\n");
      Coeffs coeffs;
      // Nasty error prone memcpy. It expects 3 32-bit IEEE-754 in
      // little-endian. Coeffs is a struct of 3 floats, but is it aligned? Does
      // it have padding?
      memcpy(&coeffs, data + 1, sizeof(Coeffs));
      Serial.printf("[ble] Coeffs: %f, %f, %f\n", coeffs.a2, coeffs.a1,
                    coeffs.a0);
      ble.config_->SetCoeffs(coeffs);
      suspendLoop();
      ble.config_->CommitToFlash();
      resumeLoop();
      WriteToConfigChar(*ble.config_, ble.cfg_char_);
      return;
    }
    case 0x03: {
      Serial.printf("[ble] Will set realtime run %d\n", data[1]);
      ble.config_->SetRealtimeRun(data[1] != 0);
      WriteToConfigChar(*ble.config_, ble.cfg_char_);
      return;
    }
    case 0x04: {
      std::string name((char*)data + 1, len - 1);
      Serial.printf("[ble] Will set name to: %s (len %d)\n", name.c_str(),
                    len - 1);
      ble.config_->SetName(name);
      suspendLoop();
      ble.config_->CommitToFlash();
      resumeLoop();
      WriteToConfigChar(*ble.config_, ble.cfg_char_);
      return;
    }
    case 0x05: {
      uint16_t sleep_ms = data[1] << 8 | data[2];
      Serial.printf("[ble] Will set sleep to: %d\n", sleep_ms);
      suspendLoop();
      ble.config_->SetSleepMS(sleep_ms);
      ble.config_->CommitToFlash();
      WriteToConfigChar(*ble.config_, ble.cfg_char_);
      resumeLoop();
      return;
    }
    case 0x06: {
      Serial.println("[ble] Will reset device");
      sd_nvic_SystemReset();
      return;
    }
    case 0x07: {
      Serial.println("[ble] Will set IMU offsets");
      // Expects 6 16-bit signed integers in little-endian.
      if (len != 1 + 6 * 2) {
        Serial.printf("[ble] Invalid data length for IMU offsets: %d\n", len);
        return;
      }
      IMU::Offsets offsets;
      // I'm sorry again.
      memcpy(&offsets, data + 1, sizeof(IMU::Offsets));
      Serial.printf("[ble] Got IMU offsets: %d, %d, %d, %d, %d, %d\n",
                    offsets.accel_x, offsets.accel_y, offsets.accel_z,
                    offsets.gyro_x, offsets.gyro_y, offsets.gyro_z);
      suspendLoop();
      ble.config_->SetIMUOffsets(offsets);
      ble.config_->CommitToFlash();
      WriteToConfigChar(*ble.config_, ble.cfg_char_);
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

  // Request MTU exchange.
  bool mtu_exchange_res =
      Bluefruit.Connection(conn_handle)->requestMtuExchange(247);
  Serial.printf("[ble] MTU exchange request result: %d\n", mtu_exchange_res);

  client_ctx[conn_handle].conn_handle = conn_handle;
  xTimerStart(client_ctx[conn_handle].change_conn_interval_timer, 0);

  Serial.printf("[ble] Connection callback. Conn handle: %u, #clients: %d\n",
                conn_handle, ble.n_conns_);
}

void BLE::DisconnCallback(uint16_t conn_handle, uint8_t reason) {
  BLE& ble = BLE::GetInstance();
  // Race?
  --ble.n_conns_;
  Serial.printf("[ble] Disconnect callback. Conn handle: %u, #clients: %d\n",
                conn_handle, ble.n_conns_);
  ble.config_->SetRealtimeRun(false);

  xTimerStop(client_ctx[conn_handle].change_conn_interval_timer, 0);
  client_ctx[conn_handle].conn_handle = BLE_CONN_HANDLE_INVALID;

  // Keep or resume advertising.
  // Stop first so we re-kick the fast advertising.
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.start(0);
}

}  // namespace sugarboat