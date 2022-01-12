#include "sugarboat/config.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

namespace sugarboat {
namespace {
constexpr char kConfigFilename[] = "config.dat";
// Size of the buffer used to read/write config files to the filesystem.
constexpr size_t kBuffSize = 15;
}  // namespace

size_t Config::Serialize(uint8_t *buf, size_t buf_size) {
  buf[0] = version_;
  // Bytes 1 & 2 are reserved.
  memcpy(buf + 3, &imu_offsets, sizeof(imu_offsets));
  return kBuffSize;
}

Config Config::Deserialize(uint8_t *buf, size_t buff_size) {
  Config config;

  if (buff_size < kBuffSize) {
    Serial.printf(
        "[config] Not enough data to deserialize config. Had %d. Returning new "
        "one\n",
        buff_size);
    return config;
  }
  config.version_ = buf[0];
  // Bytes 1 & 2 are reserved.
  memcpy(&config.imu_offsets, buf + 3, sizeof(IMU::Offsets));

  auto &offsets = config.imu_offsets;
  Serial.printf("Read config. Offsets: %d %d %d %d %d %d\n", offsets.accel_x,
                offsets.accel_y, offsets.accel_z, offsets.gyro_x,
                offsets.gyro_y, offsets.gyro_z);
  return config;
}

Config Config::ReadFromFlash() {
  if (!InternalFS.begin()) {
    Serial.println(
        "[config] Error initializing the filesystem. Returning new config\n");
    return Config();
  }

  Adafruit_LittleFS_Namespace::File file(InternalFS);

  if (!file.open(kConfigFilename, Adafruit_LittleFS_Namespace::FILE_O_READ)) {
    Serial.println(
        "[config] No existing config in flash. Returning new config\n");
    return Config();
  }

  uint8_t buff[kBuffSize] = {0x00};

  size_t read_bytes;
  if ((read_bytes = file.read(buff, sizeof(buff))) < 0) {
    Serial.println("[config] Error reading from flash. Returning new config\n");
    file.close();
    return Config();
  }

  file.close();
  // for (int x = 0; x < read_bytes; x++) {
  //   Serial.printf("%02x ", buff[x]);
  // }
  // Serial.println();
  return Deserialize(buff, read_bytes);
}

bool Config::CommitToFlash() {
  uint8_t buff[kBuffSize] = {0x00};
  size_t data_size = Serialize(buff, sizeof(buff));

  // Serial.printf("Will write Buffer: \n");
  // for (int x = 0; x < kBuffSize; x++) {
  //   Serial.printf("%02x ", buff[x]);
  // }
  // Serial.println();

  if (!InternalFS.begin()) {
    Serial.println("[config] Error initializing the filesystem\n");
    return false;
  }

  // I would have expected opening the file with FILE_O_WRITE would truncate it,
  // but I'm seeing otherwise. Explicitly remove it before hand seems to work.
  if (!InternalFS.remove(kConfigFilename)) {
    Serial.printf("[config] Unable to remove config file - first run?\n");
    // return false;
  }

  Adafruit_LittleFS_Namespace::File file(InternalFS);
  if (!file.open(kConfigFilename, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    Serial.println("[config] Error opening file for writing\n");
    return false;
  }

  size_t written_len;
  if ((written_len = file.write(buff, data_size)) < data_size) {
    Serial.printf("[config] error writing file\n");
    file.close();
    return false;
  }

  file.close();
  return true;
}

}  // namespace sugarboat