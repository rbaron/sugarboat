#ifndef _SUGARBOAT_SHT30_H_
#define _SUGARBOAT_SHT30_H_

#include <Adafruit_SHT31.h>

namespace sugarboat {

class SHT30 {
 public:
  SHT30() : sht31_(Adafruit_SHT31()) {}

  bool Init();
  float GetTemp() const;
  float GetHumi() const;

 private:
  mutable Adafruit_SHT31 sht31_;
};
}  // namespace sugarboat
#endif  // _SUGARBOAT_SHT30_H_