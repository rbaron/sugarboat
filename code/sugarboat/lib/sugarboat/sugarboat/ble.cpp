#include "sugarboat/ble.h"

namespace sugarboat {
namespace {
constexpr int kMaxConnections = 2;

int kConnections = 0;

void ConnCallback(uint16_t conn_handle) {
  digitalToggle(LED_BUILTIN);
  if (++kConnections < kMaxConnections) {
    Bluefruit.Advertising.start(0);
  }
}

void DisconnCallback(uint16_t conn_handle, uint8_t reason) {
  --kConnections;
}

void CfgCharWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr,
                          uint8_t* data, uint16_t len) {
  Serial.printf("[ble] CFG write callback: %s\n", data);
  digitalToggle(LED_BUILTIN);
}

}  // namespace

bool BLE::Init() {
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
  cfg_char_.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  cfg_char_.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  cfg_char_.setWriteCallback(CfgCharWriteCallback);
  if (cfg_char_.begin()) {
    Serial.println("[ble] Error initializing BLE config characteristic");
    return false;
  }

  return true;
}

bool BLE::StartAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  Bluefruit.Advertising.addService(bleuart_);
  Bluefruit.Advertising.addService(cfg_service_);

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

}  // namespace sugarboat