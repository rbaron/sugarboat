#include <bluefruit.h>

const char sensor_service_uuid[37] = "9b7d5c6f-a8ca-4080-9290-d4afb5ac64a3";
const char sensor_char_uuid[37] = "527d0f9b-db66-48c5-9089-071e1a795b6f";
BLEClientService        sensor_service(sensor_service_uuid);
BLEClientCharacteristic sensor_char(sensor_char_uuid);

void setup()
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);
  Serial.println("Scanning for sugarboats...");

  Bluefruit.begin(/*prph_count=*/0, /*central_count=*/1);
  Bluefruit.setName("sugarboat Arduino Client");
  Bluefruit.setTxPower(4);

  sensor_service.begin();

  sensor_char.setNotifyCallback(notify_callback);
  sensor_char.begin();

  Bluefruit.Central.setDisconnectCallback(disconnect_callback);
  Bluefruit.Central.setConnectCallback(connect_callback);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.filterUuid(sensor_service.uuid);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.start(0);                   // // 0 = Don't stop scanning after n seconds
}

void loop() {}

void scan_callback(ble_gap_evt_adv_report_t* report) {
  
  Bluefruit.Central.connect(report);

  // If we want to connect to more than one sugarboat.
//  Bluefruit.Scanner.resume();
}

void connect_callback(uint16_t conn_handle)
{
  Serial.println("[connect_callback] Connected");

  if (!sensor_service.discover(conn_handle) ) {
    Serial.println("[connect_callback] Did not find sensor service");
    Bluefruit.disconnect(conn_handle);
    return;
  }

  Serial.println("[connect_callback] Found the sensor service. Discovering the sensor characteristic...");
  
  if (!sensor_char.discover()) {
    Serial.println("[connect_callback] Sensor characteristic not found");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  
  Serial.println("[connect_callback] Sensor characteristic found");

  if (sensor_char.enableNotify()) {
    Serial.println("[connect_callback] Notify enabled for the sensor characteristic");
  } else {
    Serial.println("[connect_callback] Could not enable notify for the sensor characteristic");
    return;
  }
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  Serial.print("[disconnect_callback] Disconnected, reason = 0x");
  Serial.println(reason, HEX);
  Bluefruit.Scanner.resume();
}

void notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  print_sensor_data(data, len);
}

template<typename Type>
Type parse_bytes(uint8_t *buf, uint16_t idx) {
  Type ret;
  memcpy(&ret, buf + idx, sizeof(Type));
  return ret;
}

void print_sensor_data(uint8_t *data, uint16_t len) {
  if (len < 14) {
    Serial.printf("[print_sensor_data] Not enough data was received: %d bytes\n", len);
    return;
  }
  float tilt_angle = parse_bytes<int16_t>(data, 2) / 10.0f;
  float brix = parse_bytes<int16_t>(data, 4) / 100.0f;
  float specific_gravity = parse_bytes<int16_t>(data, 6) / 1000.0f;
  float temp_celcius = parse_bytes<int16_t>(data, 8) / 100.0f;
  float rel_humidity = 100.0 * parse_bytes<uint16_t>(data, 10) / ((float) UINT16_MAX);
  float batt_voltage = parse_bytes<uint16_t>(data, 12) / 1000.0f;
  
  Serial.printf("[print_sensor_data] Tilt angle: %.2f °, Brix: %.2f °Bx, SG: %.2f, Temp: %.2f °C, Humi: %.2f %, Batt: %.2f V\n",
    tilt_angle,
    brix,
    specific_gravity,
    temp_celcius,
    rel_humidity,
    batt_voltage);
}
