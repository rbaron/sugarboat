#include "sugarboat/mpu6050.h"

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
#include "sugarboat/config.h"

namespace sugarboat {
namespace {

int InitMPULowPower(MPU6050& mpu) {
  // Disable DMP.
  mpu.setDMPEnabled(false);

  // Enable FIFO.
  mpu.setFIFOEnabled(true);

  // mpu_.setWakeCycleEnabled(true);
  // 5 Hz accelerometer wakeup frequency - only applies to low power mode.
  // mpu_.setWakeFrequency(0x01);
  // mpu_.setRate(0x1);
  // Use internal clock source. Requirement for low power mode.
  // mpu_.setClockSource(0x00);

  // Disable temp sensor.
  mpu.setTempSensorEnabled(false);

  // Disable gyro.
  mpu.setStandbyXGyroEnabled(true);
  mpu.setStandbyYGyroEnabled(true);
  mpu.setStandbyZGyroEnabled(true);

  // Disable gyro in FIFO.
  mpu.setXGyroFIFOEnabled(false);
  mpu.setYGyroFIFOEnabled(false);
  mpu.setZGyroFIFOEnabled(false);

  // Enable accel in FIFO.
  mpu.setAccelFIFOEnabled(true);

  // Accel wake up frequency (only in low power mode).
  mpu.setWakeFrequency(0x00);

  // Use internal clock source. Requirement for low power mode?
  mpu.setClockSource(0x0);

  return 0;
}

int InitMPUDMPMode(MPU6050& mpu) {
  // The MPU6050's mysterious DMP processor needs its firmware
  // written every time it's powered on.
  if (mpu.dmpInitialize()) {
    Serial.printf("[imu] Failed to initialize DMP\n");
    return 1;
  }

  mpu.setIntDataReadyEnabled(true);
  mpu.setDMPEnabled(true);

  return 0;
}
}  // namespace

int IMU::Init(const Offsets& offsets) {
  pinMode(FLT_MPU_INTERRUPT_PIN, INPUT);
  mpu_.initialize();

  if (!mpu_.testConnection()) {
    Serial.printf("[imu] Connection test failed\n");
    return 1;
  }

  // The MPU6050's mysterious DMP processor needs its firmware
  // written every time it's powered on.
  // if (mpu_.dmpInitialize()) {
  //   Serial.printf("[imu] Failed to initialize DMP\n");
  //   return 1;
  // }

  // mpu_.setIntDataReadyEnabled(true);

  // mpu_.setDMPEnabled(true);

  // mpu_.setDMPEnabled(false);

  mpu_.setXAccelOffset(offsets.accel_x);
  mpu_.setYAccelOffset(offsets.accel_y);
  mpu_.setZAccelOffset(offsets.accel_z);
  mpu_.setXGyroOffset(offsets.gyro_x);
  mpu_.setYGyroOffset(offsets.gyro_y);
  mpu_.setZGyroOffset(offsets.gyro_z);

  // InitMPULowPower(mpu_);
  InitMPUDMPMode(mpu_);

  Serial.printf("[imu] Initialized!\n");
  return 0;
}

int IMU::DeInit() {
  return 0;
}

void IMU::Sleep() {
  mpu_.setDMPEnabled(false);
  mpu_.setSleepEnabled(true);
}

void IMU::WakeUp() {
  mpu_.setSleepEnabled(false);
  mpu_.setDMPEnabled(true);
}

IMU::Offsets IMU::Calibrate() {
  mpu_.setXAccelOffset(0);
  mpu_.setYAccelOffset(0);
  mpu_.setZAccelOffset(0);
  mpu_.setXGyroOffset(0);
  mpu_.setYGyroOffset(0);
  mpu_.setZGyroOffset(0);

  mpu_.CalibrateAccel(10);
  mpu_.CalibrateGyro(10);
  mpu_.PrintActiveOffsets();

  IMU::Offsets offsets;

  offsets.accel_x = mpu_.getXAccelOffset();
  offsets.accel_y = mpu_.getYAccelOffset();
  offsets.accel_z = mpu_.getZAccelOffset();
  offsets.gyro_x = mpu_.getXGyroOffset();
  offsets.gyro_y = mpu_.getYGyroOffset();
  offsets.gyro_z = mpu_.getZGyroOffset();
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

IMU::Orientation IMU::GetOrientation() {
  while (!mpu_.getFIFOCount())
    ;

  Orientation orientation;

  if (!mpu_.dmpGetCurrentFIFOPacket(fifo_buffer_)) {
    Serial.println("[imu] Unable to get packet from fifo buffer");
    return orientation;
  }

  if (mpu_.dmpGetQuaternion(&quaternion_, fifo_buffer_)) {
    Serial.println("[imu] Error in dmpGetQuaternion");
    return orientation;
  }
  orientation.quaternion.w = quaternion_.w;
  orientation.quaternion.x = quaternion_.x;
  orientation.quaternion.y = quaternion_.y;
  orientation.quaternion.z = quaternion_.z;

  if (mpu_.dmpGetEuler((float*)&orientation.euler_angles, &quaternion_)) {
    Serial.println("[imu] Error in dmpGetEuler");
    return orientation;
  }
  orientation.euler_angles.psi *= 180.0f / PI;
  orientation.euler_angles.theta *= 180.0f / PI;
  orientation.euler_angles.phi *= 180.0f / PI;

  return orientation;
}

}  // namespace sugarboat