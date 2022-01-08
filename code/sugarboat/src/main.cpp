#include <Adafruit_LittleFS.h>
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>

#include "Wire.h"
#include "sugarboat/config.h"
#include "sugarboat/mpu6050.h"

sugarboat::IMU imu;
sugarboat::Config config;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  while (!Serial)
    ;
  delay(1000);

  Serial.println("ok!\n");
  config = sugarboat::Config::ReadFromFlash();
  const sugarboat::IMU::Offsets &offsets = config.GetIMUOffsets();
  Serial.printf("Initial offsets: %d %d %d %d %d %d\n", offsets.accel_x,
                offsets.accel_y, offsets.accel_z, offsets.gyro_x,
                offsets.gyro_y, offsets.gyro_z);

  sugarboat::IMU::Offsets imu_offsets = imu.Calibrate();
  config.SetIMUOffsets(imu_offsets);
  config.CommitToFlash();

  // if (imu.Init()) {
  //   Serial.printf("[main] Error initializing imu");
  //   while (true)
  //     ;
  // }

  // const sugarboat::IMU::Offsets &offsets2 = config.GetIMUOffsets();
  // Serial.printf("Done. Offsets: %d %d %d %d %d %d\n", offsets2.accel_x,
  //               offsets2.accel_y, offsets2.accel_z, offsets2.gyro_x,
  //               offsets2.gyro_y, offsets2.gyro_z);
}

void loop() {
  // float angle = imu.GetTilt();
  // Serial.printf("Got angle: %.2f\n", angle);
  delay(100);
}