#include <Adafruit_LittleFS.h>
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>

#include "Wire.h"
#include "sugarboat/ble.h"
#include "sugarboat/config.h"
#include "sugarboat/logger.h"
#include "sugarboat/mpu6050.h"

sugarboat::BLE ble;
sugarboat::Config config;
sugarboat::Logger logger(Serial, ble);
sugarboat::IMU imu;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  while (!Serial)
    ;
  delay(1000);

  config = sugarboat::Config::ReadFromFlash();
  const sugarboat::IMU::Offsets &offsets = config.GetIMUOffsets();
  logger.printf("[main] IMU offsets from flash: %d %d %d %d %d %d\n",
                offsets.accel_x, offsets.accel_y, offsets.accel_z,
                offsets.gyro_x, offsets.gyro_y, offsets.gyro_z);

  if (imu.Init(config.GetIMUOffsets())) {
    logger.printf("[main] Error initializing imu");
    while (true)
      ;
  }

  if (!ble.Init()) {
    logger.println("[main] Error intializing BLE");
    while (true)
      ;
  }
  ble.StartAdv();
}

int n = 0;
void loop() {
  float angle = imu.GetTilt();
  // logger.printf("Angle: %.2f\n", angle);
  delay(1000);
}