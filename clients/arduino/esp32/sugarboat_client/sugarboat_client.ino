
#include "BLEDevice.h"
#include <memory>

static constexpr char kSugarboatMACAddr[] = "dc:a6:64:68:ef:5f";
static  BLEUUID kServiceUUID("9b7d5c6f-a8ca-4080-9290-d4afb5ac64a3");
static  BLEUUID    kCharUUID("527d0f9b-db66-48c5-9089-071e1a795b6f");

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
//static BLEAdvertisedDevice* myDevice;
static std::unique_ptr<BLEAdvertisedDevice> myDevice;

template<typename Type>
Type parse_bytes(uint8_t *buf, uint16_t idx) {
  Type ret;
  memcpy(&ret, buf + idx, sizeof(Type));
  return ret;
}

static void handleSugarboatSensorData(uint8_t *data, size_t len) {
    if (len < 14) {
    Serial.printf("[ble][handleSugarboatSensorData] Not enough data was received: %d bytes\n", len);
    return;
  }
  float tilt_angle = parse_bytes<int16_t>(data, 2) / 10.0f;
  float brix = parse_bytes<int16_t>(data, 4) / 100.0f;
  float specific_gravity = parse_bytes<int16_t>(data, 6) / 1000.0f;
  float temp_celcius = parse_bytes<int16_t>(data, 8) / 100.0f;
  float rel_humidity = 100.0 * parse_bytes<uint16_t>(data, 10) / ((float) UINT16_MAX);
  float batt_voltage = parse_bytes<uint16_t>(data, 12) / 1000.0f;
  
  Serial.printf("[ble][handleSugarboatSensorData] Tilt angle: %.2f °, Brix: %.2f °Bx, SG: %.2f, Temp: %.2f °C, Humi: %.2f %, Batt: %.2f V\n",
    tilt_angle,
    brix,
    specific_gravity,
    temp_celcius,
    rel_humidity,
    batt_voltage);
}

static void startScanning() {
  BLEDevice::getScan()->start(5, scanCompleteCB, false);
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  return handleSugarboatSensorData(pData, length);
}


void scanCompleteCB(BLEScanResults results) {
  Serial.println("[ble] Scan complete. Restarting scan.");
  startScanning();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("[ble] Connected!");
    connected = true;
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("[ble] Disconnected. Will resume scanning");
    connected = false;
    startScanning();
  }
};

static MyClientCallback client_callbacks;

bool connectToServer() {
    if (connected) {
      Serial.println("[ble] Already connected - skipping new connection");
      return true;
    }
    
    Serial.printf("[ble] Connecting to %s\n", myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(&client_callbacks);
    pClient->connect(myDevice.get());
    Serial.println("[ble] Connected to sugarboat");

    BLERemoteService* pRemoteService = pClient->getService(kServiceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("[ble] Failed to find our service UUID. Disconnecting.");
      pClient->disconnect();
      return false;
    }
    Serial.println("[ble] Service found");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(kCharUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.printf("[ble] Char %s not found. Disconnecting.\n", kCharUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println("[ble] Characteristic found");

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
    }

    if(pRemoteCharacteristic->canNotify()) {
      Serial.println("[ble] Registering for notify events");
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
    return true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
//    Serial.printf("BLE Advertised Device found: %s\n", advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(kServiceUUID)) {
      const std::string found_addr = advertisedDevice.getAddress().toString();
      Serial.printf("[ble] Found a sugarboat @ %s\n", found_addr.c_str());
      if (found_addr != kSugarboatMACAddr) {
        Serial.printf("[ble] This is not the sugarboat we're looking for! Continuing scanning...\n");
        return;
      }
      BLEDevice::getScan()->stop();
//      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      myDevice = std::unique_ptr<BLEAdvertisedDevice>(new BLEAdvertisedDevice(advertisedDevice));
      doConnect = true;
      // TODO: calling connectToServer() directly here causes it to stop execution weirdly. Why?
      // connectToServer();
    }
  }
};

static MyAdvertisedDeviceCallbacks advCallbacks;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(&advCallbacks);
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  startScanning();
}


void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("[ble] Connected!");
    } else {
      Serial.println("[ble][main] Connection failed. Starting scanning...");
      startScanning();
    }
    doConnect = false;
  }
  
  delay(1000);
}
