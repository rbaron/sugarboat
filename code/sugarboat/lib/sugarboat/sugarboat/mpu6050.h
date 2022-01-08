#ifndef _SUGARBOAT_MPU6050_H_
#define _SUGARBOAT_MPU6050_H_

#include "MPU6050_6Axis_MotionApps612.h"

#define FLT_MPU_INTERRUPT_PIN 12

namespace sugarboat {

class IMU {
 public:
  struct Offsets {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
  };

  IMU() {}
  IMU(const IMU &other) = delete;
  IMU &operator=(const IMU &other) = delete;

  int Init(const Offsets &offsets);
  int DeInit();
  Offsets Calibrate();
  float GetTilt();

 private:
  MPU6050 mpu_;
  uint8_t fifo_buffer_[64];
  Quaternion quaternion_;
};
}  // namespace sugarboat
#endif  // _SUGARBOAT_MPU6050_H_