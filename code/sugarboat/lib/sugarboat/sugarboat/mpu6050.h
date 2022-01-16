#ifndef _SUGARBOAT_MPU6050_H_
#define _SUGARBOAT_MPU6050_H_

#include "MPU6050_6Axis_MotionApps612.h"

// #define FLT_MPU_INTERRUPT_PIN 12
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

  struct IMUQuaternion {
    float w;
    float x;
    float y;
    float z;
  };

  struct EulerAngles {
    float psi;
    float theta;
    float phi;
  };

  struct Orientation {
    IMUQuaternion quaternion;
    EulerAngles euler_angles;
  };

  IMU() {}
  IMU(const IMU &other) = delete;
  IMU &operator=(const IMU &other) = delete;

  int Init(const Offsets &offsets);
  int DeInit();
  Offsets Calibrate();
  float GetTilt();
  Orientation GetOrientation();

 private:
  MPU6050 mpu_;
  uint8_t fifo_buffer_[64];
  Quaternion quaternion_;
};
}  // namespace sugarboat
#endif  // _SUGARBOAT_MPU6050_H_