#ifndef _FLOATY_MPU6050_H_
#define _FLOATY_MPU6050_H_

#include "MPU6050_6Axis_MotionApps612.h"

#define FLT_MPU_INTERRUPT_PIN 12

namespace floaty {

class IMU {
 public:
  IMU() {}
  IMU(const IMU &other) = delete;
  IMU &operator=(const IMU &other) = delete;

  int Init();
  int DeInit();
  float GetTilt();

 private:
  MPU6050 mpu_;
  uint8_t fifo_buffer_[64];
  Quaternion quaternion_;
};
}  // namespace floaty
#endif  // _FLOATY_MPU6050_H_