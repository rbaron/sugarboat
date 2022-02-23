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
  void Sleep();
  void WakeUp();
  Offsets Calibrate();
  float GetTilt();
  Orientation GetOrientation();

  void Loop() {
    int fifo_count = mpu_.getFIFOCount();
    Serial.printf("[mpu] FIFO count: %d\n", fifo_count);
    int16_t x, y, z;
    mpu_.getAcceleration(&x, &y, &z);
    float angle = 180.0f * atan2(y, z) / PI;
    Serial.printf("angle: %.2f, accel x: %d, y: %d, z: %d\n", angle, x, y, z);
    // for (int i = 0; i < fifo_count; i++) {
    //   uint8_t fifo_byte = mpu_.getFIFOByte();
    //   Serial.printf("\tFIFO BYTE: 0x%02x\n", fifo_byte);
    // }
  }

 private:
  MPU6050 mpu_;
  uint8_t fifo_buffer_[64];
  Quaternion quaternion_;
};
}  // namespace sugarboat
#endif  // _SUGARBOAT_MPU6050_H_