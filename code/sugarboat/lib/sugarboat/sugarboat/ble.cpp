#include "sugarboat/ble.h"

namespace sugarboat {
namespace {
constexpr int kMaxConnections = 2;

int kConnections = 0;

void ConnCallback(uint16_t conn_handle) {
  if (++kConnections < kMaxConnections) {
    Bluefruit.Advertising.start(0);
  }
}

void DisconnCallback(uint16_t conn_handle, uint8_t reason) {
  --kConnections;
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

  return true;
}

bool BLE::StartAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  Bluefruit.Advertising.addService(bleuart_);

  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  // Advertisement interval in units of 625 us. For estimating impact in battery
  // life, use
  // https://devzone.nordicsemi.com/nordic/power/w/opp/2/online-power-profiler-for-ble.
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);

  return true;
}

}  // namespace sugarboat