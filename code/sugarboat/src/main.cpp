
#include <Adafruit_LittleFS.h>
#include <Adafruit_TinyUSB.h>
#include <Arduino.h>
#include <bluefruit.h>

#include "Wire.h"
#include "sugarboat/ble.h"
#include "sugarboat/config.h"
#include "sugarboat/mpu6050.h"
#include "sugarboat/sensor_data.h"
#include "sugarboat/sht30.h"

sugarboat::Config config;
sugarboat::BLE &ble = sugarboat::BLE::GetInstance();
sugarboat::IMU imu;
sugarboat::SHT30 sht30;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

#define SGRBT_LED_PIN 16
#define SGRBT_SDA_PIN 6
#define SGRBT_SCL_PIN 10
#define SGRBT_BAT_SEN_PIN 18

// Baseline: 17 uA
// With accel in low power mode: 30 uA
// With BLE + accel lo po: 40 uA

void setup() {
  // Uses ~40uA!
  // pinMode(SGRBT_BAT_SEN_PIN, INPUT);

  pinMode(SGRBT_LED_PIN, OUTPUT);

  digitalWrite(SGRBT_LED_PIN, HIGH);

  Wire.setPins(SGRBT_SDA_PIN, SGRBT_SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  // Uncomment to block the execution until the USB serial port is open.
  while (!Serial)
    ;
  delay(500);

  if (!sht30.Init()) {
    Serial.println("[main] Error initializing SHT30");
    while (1) delay(1);
  }

  config = sugarboat::Config::ReadFromFlash();

  sugarboat::IMU::Offsets offsets = config.GetIMUOffsets();
  Serial.printf("[main] IMU offsets from flash: %d %d %d %d %d %d\n",
                offsets.accel_x, offsets.accel_y, offsets.accel_z,
                offsets.gyro_x, offsets.gyro_y, offsets.gyro_z);

  if (imu.Init(offsets)) {
    Serial.printf("[main] Error initializing imu\n");
    while (true)
      ;
  }

  sugarboat::BLE &ble = sugarboat::BLE::GetInstance();
  if (!ble.Init(config, imu)) {
    Serial.println("[main] Error intializing BLE");
    while (true)
      ;
  }
  delay(100);
  ble.StartAdv();

  digitalWrite(SGRBT_LED_PIN, LOW);
}

sugarboat::SensorData sensor_data{0, 0, 0, 0};
void loop() {
  bool is_realtime = config.GetRealtimeRun();

  imu.WakeUp();
  sensor_data.tilt_degrees = imu.GetTilt();
  imu.Sleep();

  float batt_v = 2 * 3.6f * analogRead(SGRBT_BAT_SEN_PIN) / 1024.0f;

  sensor_data.batt_volt = batt_v;
  sensor_data.rel_humi = sht30.GetHumi();
  sensor_data.temp_celcius = sht30.GetTemp();
  delay(50);

  if (isnan(sensor_data.rel_humi)) {
    sensor_data.rel_humi = 0;
  }
  if (isnan(sensor_data.temp_celcius)) {
    sensor_data.temp_celcius = 0;
  }

  ble.InjectSensorData(sensor_data);

  // Serial.printf("Angle: %.2f Temp: %.2f, Humi: %.2f Batt: %.2f\n",
  //               sensor_data.tilt_degrees, sensor_data.temp_celcius,
  //               sensor_data.rel_humi, sensor_data.batt_volt);

  if (!is_realtime) {
    Serial.printf("[main] Delaying...\n");
    config.WaitForConfigChangeOrDelay(5000);
  }
}