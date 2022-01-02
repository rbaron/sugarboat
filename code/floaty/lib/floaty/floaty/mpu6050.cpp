#include "floaty/mpu6050.h"

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"

namespace floaty {
namespace {}  // namespace

int IMU::Init() {
  pinMode(FLT_MPU_INTERRUPT_PIN, INPUT);
  mpu_.initialize();

  if (!mpu_.testConnection()) {
    Serial.printf("[imu] Connection test failed\n");
    return 1;
  }

  // The MPU6050's mysterious DMP processor needs its firmware
  // written every time it's powered on.
  if (mpu_.dmpInitialize()) {
    Serial.printf("[imu] Failed to initialize DMP\n");
    return 1;
  }

  mpu_.setDMPEnabled(true);
  mpu_.setXAccelOffset(-1743);
  mpu_.setYAccelOffset(719);
  mpu_.setZAccelOffset(1101);
  mpu_.setXGyroOffset(100);
  mpu_.setYGyroOffset(60);
  mpu_.setZGyroOffset(1);

  Serial.printf("[imu] Initialized!\n");
  return 0;
}

int IMU::DeInit() {
  return 0;
}

float IMU::GetTilt() {
  while (!mpu_.getFIFOCount())
    ;

  if (!mpu_.dmpGetCurrentFIFOPacket(fifo_buffer_)) {
    Serial.println("[imu] Unable to get packet from fifo buffer");
    return 0.0f;
  }

  mpu_.dmpGetQuaternion(&quaternion_, fifo_buffer_);
  VectorFloat z_rotated{0, 0, 1};
  z_rotated.rotate(&quaternion_);

  // This is the angle between the reference frame's z-axis and
  // the IMU's's frame z-axis.
  // cos(theta) = dot(u, v) / (mag(u) * mag(v))
  float angle = acos(z_rotated.z / z_rotated.getMagnitude());

  // To make this angle easier to interpret, we convention that 0 degrees
  // should correspond to the sensor in the upright position (aligned with
  // gravity). To do that, we just subtract the angle from 90 degrees.
  return 90.0f - 180.0f * angle / PI;
}

}  // namespace floaty