#include "sugarboat/mpu6050.h"

#include <algorithm>
#include <vector>

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
#include "sugarboat/config.h"

namespace sugarboat {
namespace {

int InitMPULowPower(MPU6050& mpu) {
  // Disable DMP.
  mpu.setDMPEnabled(false);

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
  // mpu.setAccelFIFOEnabled(true);

  // Accel wake up frequency (only in low power mode).
  mpu.setWakeFrequency(0x02);

  // Use internal clock source. Requirement for low power mode?
  mpu.setClockSource(0x0);

  // Low power sequence.
  // Set CYCLE bit to 1 => PWR_MGMT_1[5]
  // mpu.setWakeCycleEnabled(true);
  // Set SLEEP bit to 0.
  mpu.setSleepEnabled(false);
  //  Set TEMP_DIS bit to 1 => PWR_MGMT_1[3]
  mpu.setTempSensorEnabled(true);
  // Set STBY_XG, STBY_YG, STBY_ZG bits to 1 => PWR_MGMT_2[0, 1, 2]
  mpu.setStandbyXGyroEnabled(true);
  mpu.setStandbyYGyroEnabled(true);
  mpu.setStandbyZGyroEnabled(true);

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

  mpu_.setXAccelOffset(offsets.accel_x);
  mpu_.setYAccelOffset(offsets.accel_y);
  mpu_.setZAccelOffset(offsets.accel_z);
  mpu_.setXGyroOffset(offsets.gyro_x);
  mpu_.setYGyroOffset(offsets.gyro_y);
  mpu_.setZGyroOffset(offsets.gyro_z);

  InitMPULowPower(mpu_);

  Serial.printf("[imu] Initialized!\n");
  return 0;
}

int IMU::DeInit() {
  return 0;
}

void IMU::Sleep() {
  mpu_.setSleepEnabled(true);
}

void IMU::WakeUp() {
  mpu_.setSleepEnabled(false);
}

IMU::Offsets IMU::Calibrate() {
  mpu_.setXAccelOffset(0);
  mpu_.setYAccelOffset(0);
  mpu_.setZAccelOffset(0);
  mpu_.setXGyroOffset(0);
  mpu_.setYGyroOffset(0);
  mpu_.setZGyroOffset(0);

  WakeUp();

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
  constexpr int loops = 100;
  int16_t raw_x, raw_y, raw_z;
  float sum_angle = 0;
  for (int i = 0; i < loops; i++) {
    mpu_.getAcceleration(&raw_x, &raw_y, &raw_z);
    sum_angle += atan2((float)raw_y, (float)raw_z);
    delay(10);
  }

  return 90.0f - 180.0f * (sum_angle / loops) / PI;
}

float IMU::GetTemp() {
  int16_t temp_out = mpu_.getTemperature();
  // From the MPU6050 register map datasheet.
  return temp_out / 340.0f + 36.53;
}

}  // namespace sugarboat