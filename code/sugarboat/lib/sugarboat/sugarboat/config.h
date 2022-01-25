#ifndef _SUGARBOAT_CONFIG_H_
#define _SUGARBOAT_CONFIG_H_

#include <Adafruit_LittleFS.h>

#include "sugarboat/mpu6050.h"
#include "sugarboat/sugar_scale.h"

namespace sugarboat {

// Size of the buffer used to read/write config files to the filesystem.
constexpr size_t kBuffSize = 3 + sizeof(IMU::Offsets) + sizeof(Coeffs);

class Config {
 public:
  Config() {
    x_config_semaphore_ = xSemaphoreCreateMutex();
    if (!xSemaphoreTake(x_config_semaphore_, pdMS_TO_TICKS(100))) {
      Serial.println("[config] Unable to get own semaphore");
    }
  }

  static Config ReadFromFlash();
  bool CommitToFlash();

  // Returns true if the config has changed.
  bool WaitForConfigChangeOrDelay(unsigned int delay_ms) {
    if (xSemaphoreTake(x_config_semaphore_, pdMS_TO_TICKS(delay_ms))) {
      // Immediately give the semaphore back.
      xSemaphoreGive(x_config_semaphore_);
      Serial.println(
          "[config] WaitOrConfigChange took semaphore and gave it back");
      return true;
    }
    Serial.println("[config] WaitOrConfigChange did not take the semaphore");
    return false;
  }

  const IMU::Offsets &GetIMUOffsets() const {
    return imu_offsets;
  }

  bool HasIMUOffsets() {
    return has_imu_offsets_;
  }

  void SetIMUOffsets(const IMU::Offsets &offsets) {
    imu_offsets = offsets;
    has_imu_offsets_ = true;
  }

  bool HasCoeffs() {
    return has_imu_offsets_;
  }

  const Coeffs &GetCoeffs() const {
    return coeffs_;
  }
  void SetCoeffs(const Coeffs &coeffs) {
    coeffs_ = coeffs;
    has_coeffs_ = true;
  }

  void SetRealtimeRun(bool value) {
    realtime_run_ = value;
    xSemaphoreGive(x_config_semaphore_);
    Serial.printf("[config] SetRealtimeRun gave semaphore: is inISR: %d\n",
                  isInISR());
    // Give some time for the main loop to run and take the semaphore.
    delay(10);
    if (!xSemaphoreTake(x_config_semaphore_, pdMS_TO_TICKS(100))) {
      Serial.println(
          "[config] ERROR SetRealtimeRun couldn't take the config sempahore");
      return;
    }
    Serial.println("[config] SetRealtimeRun took semaphore again");
  }

  bool GetRealtimeRun() {
    return realtime_run_;
  }

  size_t Serialize(uint8_t *buf, size_t buf_size) const;

 private:
  static Config Deserialize(uint8_t *buf, size_t buff_size);

  uint8_t version_ = 0;
  bool has_imu_offsets_ = false;
  IMU::Offsets imu_offsets;

  bool has_coeffs_ = false;
  Coeffs coeffs_;

  // Setting realtime_run_ to true will cause sensors to be read as fast as
  // possible and prevent any sort of power saving strategy. It's useful for
  // debugging and/or demos. This field is not persisted, as it should only
  // rarely be used.
  bool realtime_run_ = false;

  SemaphoreHandle_t x_config_semaphore_ = nullptr;
};
}  // namespace sugarboat

#endif  // _SUGARBOAT_CONFIG_H_