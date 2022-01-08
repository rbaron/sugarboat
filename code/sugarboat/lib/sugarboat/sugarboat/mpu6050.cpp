#include "sugarboat/mpu6050.h"

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
#include "sugarboat/config.h"

namespace sugarboat {
namespace {}  // namespace

int IMU::Init(const Offsets& offsets) {
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
  // mpu_.setXAccelOffset(0);
  // mpu_.setYAccelOffset(0);
  // mpu_.setZAccelOffset(0);
  // mpu_.setXGyroOffset(0);
  // mpu_.setYGyroOffset(0);
  // mpu_.setZGyroOffset(0);
  // mpu_.setXAccelOffset(-1743);
  // mpu_.setYAccelOffset(719);
  // mpu_.setZAccelOffset(1101);
  // mpu_.setXGyroOffset(100);
  // mpu_.setYGyroOffset(60);
  // mpu_.setZGyroOffset(1);
  mpu_.setXAccelOffset(offsets.accel_x);
  mpu_.setYAccelOffset(offsets.accel_y);
  mpu_.setZAccelOffset(offsets.accel_z);
  mpu_.setXGyroOffset(offsets.gyro_x);
  mpu_.setYGyroOffset(offsets.gyro_y);
  mpu_.setZGyroOffset(offsets.gyro_z);

  Serial.printf("[imu] Initialized!\n");
  return 0;
}

int IMU::DeInit() {
  return 0;
}

IMU::Offsets IMU::Calibrate() {
  // mpu_.setXAccelOffset(0);
  // mpu_.setYAccelOffset(0);
  // mpu_.setZAccelOffset(0);
  // mpu_.setXGyroOffset(0);
  // mpu_.setYGyroOffset(0);
  // mpu_.setZGyroOffset(0);

  // mpu_.CalibrateAccel(1);
  // mpu_.CalibrateGyro(1);
  // mpu_.PrintActiveOffsets();

  IMU::Offsets offsets;

  offsets.accel_x = (-1743);
  offsets.accel_y = (719);
  offsets.accel_z = (1101);
  offsets.gyro_x = (100);
  offsets.gyro_y = (60);
  offsets.gyro_z = (1);

  // offsets.accel_x = mpu_.getXAccelOffset();
  // offsets.accel_y = mpu_.getYAccelOffset();
  // offsets.accel_z = mpu_.getZAccelOffset();
  // offsets.gyro_x = mpu_.getXGyroOffset();
  // offsets.gyro_y = mpu_.getYGyroOffset();
  // offsets.gyro_z = mpu_.getZGyroOffset();
  return offsets;
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

}  // namespace sugarboat