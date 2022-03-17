#include "sugarboat/config.h"

#include <Adafruit_LittleFS.h>
#include <ArduinoJson.h>
#include <InternalFileSystem.h>

namespace sugarboat {
namespace {
constexpr char kConfigFilename[] = "config-test.json";
}  // namespace

size_t Config::Serialize(Stream &stream) const {
  StaticJsonDocument<1024> doc;
  doc["version"] = version_;
  doc["has_imu_offsets"] = has_imu_offsets_;
  JsonObject imu_offsets_obj = doc.createNestedObject("imu_offsets");
  imu_offsets_obj["accel_x"] = imu_offsets.accel_x;
  imu_offsets_obj["accel_y"] = imu_offsets.accel_y;
  imu_offsets_obj["accel_z"] = imu_offsets.accel_z;
  imu_offsets_obj["gyro_x"] = imu_offsets.gyro_x;
  imu_offsets_obj["gyro_y"] = imu_offsets.gyro_y;
  imu_offsets_obj["gyro_z"] = imu_offsets.gyro_z;

  doc["has_coeffs"] = has_coeffs_;
  JsonObject coeffs_obj = doc.createNestedObject("coeffs");
  coeffs_obj["a0"] = coeffs_.a0;
  coeffs_obj["a1"] = coeffs_.a1;
  coeffs_obj["a2"] = coeffs_.a2;

  doc["name"] = name_;
  doc["sleep_ms"] = sleep_ms_;

  Serial.println("[config] Serialized:");
  serializeJsonPretty(doc, Serial);

  return serializeJson(doc, stream);
}

Config Config::Deserialize(Stream &stream) {
  Config config;

  StaticJsonDocument<2048> doc;
  DeserializationError status = deserializeJson(doc, stream);
  if (status != DeserializationError::Ok) {
    Serial.printf(
        "[config] Unable to deserialize config: %d. Returning a new one\n",
        status);
    return config;
  }

  Serial.println("[config] Deserialized:");
  serializeJsonPretty(doc, Serial);
  Serial.println();

  config.has_coeffs_ = doc["has_coeffs"];
  if (config.has_coeffs_) {
    config.coeffs_.a0 = doc["coeffs"]["a0"];
    config.coeffs_.a1 = doc["coeffs"]["a1"];
    config.coeffs_.a2 = doc["coeffs"]["a2"];
  }

  config.has_imu_offsets_ = doc["has_imu_offsets"];
  if (config.has_imu_offsets_) {
    config.imu_offsets.accel_x = doc["imu_offsets"]["accel_x"];
    config.imu_offsets.accel_y = doc["imu_offsets"]["accel_y"];
    config.imu_offsets.accel_z = doc["imu_offsets"]["accel_z"];
    config.imu_offsets.gyro_x = doc["imu_offsets"]["gyro_x"];
    config.imu_offsets.gyro_y = doc["imu_offsets"]["gyro_y"];
    config.imu_offsets.gyro_z = doc["imu_offsets"]["gyro_z"];
  }

  if (doc.containsKey("name")) {
    config.name_ = doc["name"].as<std::string>();
  }
  if (doc.containsKey("sleep_ms")) {
    config.sleep_ms_ = doc["sleep_ms"];
  }

  return config;
}

Config Config::ReadFromFlash() {
  if (!InternalFS.begin()) {
    Serial.println(
        "[config] Error initializing the filesystem. Returning new config");
    return Config();
  }

  Adafruit_LittleFS_Namespace::File file(InternalFS);

  if (!file.open(kConfigFilename, Adafruit_LittleFS_Namespace::FILE_O_READ)) {
    Serial.println(
        "[config] No existing config in flash. Returning new config");
    return Config();
  }

  return Deserialize(file);
}

bool Config::CommitToFlash() {
  Serial.println("[config] Will commit to flash.");
  if (!InternalFS.begin()) {
    Serial.println("[config] Error initializing the filesystem\n");
    return false;
  }

  // I would have expected opening the file with FILE_O_WRITE would truncate it,
  // but I'm seeing otherwise. Explicitly remove it before hand seems to work.
  if (!InternalFS.remove(kConfigFilename)) {
    Serial.printf("[config] Unable to remove config file - first run?\n");
  }

  Adafruit_LittleFS_Namespace::File file(InternalFS);
  if (!file.open(kConfigFilename, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    Serial.println("[config] Error opening file for writing\n");
    return false;
  }

  if (Serialize(file) <= 0) {
    Serial.printf("[config] Error serializing to file\n");
    file.close();
    return false;
  }

  file.close();

  return true;
}

}  // namespace sugarboat