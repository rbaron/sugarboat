#ifndef _SUGARBOAT_SENSOR_DATA_H_
#define _SUGARBOAT_SENSOR_DATA_H_

namespace sugarboat {

struct SensorData {
  // Angle in milliradians.
  int16_t angle_mrad;
  // Temperature in milli degrees Celcius.
  int16_t temp_mcelcius;
  // Relative humidity in [0, UINT16__MAX].
  int16_t humi;
};

}  // namespace sugarboat
#endif  // _SUGARBOAT_SENSOR_DATA_H_