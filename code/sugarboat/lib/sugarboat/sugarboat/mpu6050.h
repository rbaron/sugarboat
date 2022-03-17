#ifndef _SUGARBOAT_MPU6050_H_
#define _SUGARBOAT_MPU6050_H_

#include "MPU6050_6Axis_MotionApps612.h"

#define FLT_MPU_INTERRUPT_PIN 4

namespace sugarboat {

class IMU {
 public:
  struct Offsets {
    int16_t accel_x = 0;
    int16_t accel_y = 0;
    int16_t accel_z = 0;
    int16_t gyro_x = 0;
    int16_t gyro_y = 0;
    int16_t gyro_z = 0;
  };

  IMU() {}
  IMU(const IMU &other) = delete;
  IMU &operator=(const IMU &other) = delete;

  int Init(const Offsets &offsets);
  int DeInit();
  void Sleep();
  void WakeUp();
  Offsets Calibrate();
  float GetTilt();
  float GetTemp();

 private:
  MPU6050 mpu_;
};
}  // namespace sugarboat
#endif  // _SUGARBOAT_MPU6050_H_