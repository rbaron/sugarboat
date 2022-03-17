#ifndef _SUGARBOAT_SENSOR_DATA_H_
#define _SUGARBOAT_SENSOR_DATA_H_

namespace sugarboat {

struct SensorData {
  float tilt_degrees;
  float temp_celsius;
  // Relative humidity in [0.0, 1.0].
  float rel_humi;
  // Battery voltage in Volts.
  float batt_volt;
  // MPU6050's temperature.
  float mpu_temp_celsius;
};

}  // namespace sugarboat
#endif  // _SUGARBOAT_SENSOR_DATA_H_