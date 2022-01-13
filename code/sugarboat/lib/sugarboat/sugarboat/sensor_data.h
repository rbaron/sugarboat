#ifndef _SUGARBOAT_SENSOR_DATA_H_
#define _SUGARBOAT_SENSOR_DATA_H_

namespace sugarboat {

struct SensorData {
  // Angle in milliradians.
  // int16_t angle_mrad;
  float tilt_degrees;
  // Temperature in milli degrees Celcius.
  // int16_t temp_mcelcius;
  float temp_celcius;
  // Relative humidity in [0, UINT16__MAX].
  float rel_humi;
  float batt_volt;
};

}  // namespace sugarboat
#endif  // _SUGARBOAT_SENSOR_DATA_H_