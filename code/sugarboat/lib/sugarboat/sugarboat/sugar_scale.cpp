#include "sugarboat/sugar_scale.h"

#include <math.h>

namespace sugarboat {

float CalculateBrix(const Coeffs& coeffs, float tilt_angle) {
  return coeffs.a2 * pow(tilt_angle, 2) + coeffs.a1 * tilt_angle + coeffs.a0;
}

float BrixToSG(float brix) {
  return (brix / (258.6 - ((brix / 258.2) * 227.1))) + 1;
}

}  // namespace sugarboat