#include "sugarboat/sht30.h"

namespace sugarboat {

bool SHT30::Init() {
  if (!sht31_.begin(0x44)) {
    Serial.println("[main] Error initializing SHT30");
    while (1) delay(1);
  }
}
float SHT30::GetTemp() const {
  return sht31_.readTemperature();
}
float SHT30::GetHumi() const {
  return sht31_.readHumidity();
}

}  // namespace sugarboat