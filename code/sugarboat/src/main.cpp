
#include <Adafruit_LittleFS.h>
#include <Adafruit_TinyUSB.h>
#include <Arduino.h>
#include <bluefruit.h>

#include "sugarboat/ble.h"
#include "sugarboat/config.h"
// #include "sugarboat/logger.h"
#include "Wire.h"
#include "sugarboat/mpu6050.h"

sugarboat::Config config;
sugarboat::BLE &ble = sugarboat::BLE::GetInstance();
// sugarboat::Logger logger(Serial, ble);
sugarboat::IMU imu;
#define LED_BUILTIN2 16
#define SGRBT_SDA_PIN 6
#define SGRBT_SCL_PIN 10

void setup() {
  pinMode(LED_BUILTIN2, OUTPUT);

  Wire.setPins(SGRBT_SDA_PIN, SGRBT_SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  while (!Serial)
    ;
  delay(1000);
  Serial.println("Hello, world2");

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

  Serial.printf("[main] Will calibrate:\n");
  offsets = imu.Calibrate();
  Serial.printf("[main] IMU offsets from calibration: %d %d %d %d %d %d\n",
                offsets.accel_x, offsets.accel_y, offsets.accel_z,
                offsets.gyro_x, offsets.gyro_y, offsets.gyro_z);

  config.SetIMUOffsets(offsets);
  if (!config.CommitToFlash()) {
    Serial.printf("[main] Error commiting config to flash\n");
  }

  sugarboat::BLE &ble = sugarboat::BLE::GetInstance();
  if (!ble.Init(config)) {
    Serial.println("[main] Error intializing BLE");
    while (true)
      ;
  }
  ble.StartAdv();
}

void loop() {
  float angle = imu.GetTilt();
  Serial.printf("Angle: %.2f\n", angle);
  sugarboat::IMU::Orientation orientation = imu.GetOrientation();
  ble.InjectOrientationData(orientation);
  // logger.printf("w: %.2f x: %.2f y: %.2f z:%.2f\n",
  // orientation.quaternion.w,
  // //               orientation.quaternion.x, orientation.quaternion.y,
  // //               orientation.quaternion.z);
  digitalToggle(LED_BUILTIN2);
  delay(250);
}