#ifndef _SUGARBOAT_CONFIG_H_
#define _SUGARBOAT_CONFIG_H_

#include <Adafruit_LittleFS.h>

#include "sugarboat/mpu6050.h"

namespace sugarboat {
class Config {
 public:
  static Config ReadFromFlash();
  bool CommitToFlash();

  const IMU::Offsets &GetIMUOffsets() const {
    return imu_offsets;
  }

  void SetIMUOffsets(const IMU::Offsets &offsets) {
    imu_offsets = offsets;
  }

 private:
  static Config Deserialize(uint8_t *buf, size_t buff_size);
  size_t Serialize(uint8_t *buf, size_t buf_size);

  uint8_t version_;
  bool has_imu_offsets_;
  IMU::Offsets imu_offsets;
};
}  // namespace sugarboat

#endif  // _SUGARBOAT_CONFIG_H_