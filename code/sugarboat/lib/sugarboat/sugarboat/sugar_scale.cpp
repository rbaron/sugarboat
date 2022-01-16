#include "sugarboat/sugar_scale.h"

#include <math.h>

namespace sugarboat {

// float CalculateSG(const Coeffs& coeffs, float tilt_angle) {
//   return coeffs.a2 * tilt_angle * tilt_angle + coeffs.a1 * tilt_angle +
//          coeffs.a0;
// }

// // https://en.wikipedia.org/wiki/Brix
// float BrixToSG(float brix) {
//   return 182.4601 * pow(brix, 3) - 775 * pow(brix, 2) + 1262.7794 * brix -
//          669.5622;
// }

float CalculateBrix(const Coeffs& coeffs, float tilt_angle) {
  return coeffs.a2 * pow(tilt_angle, 2) + coeffs.a1 * tilt_angle + coeffs.a0;
}

float BrixToSG(float brix) {
  return (brix / (258.6 - ((brix / 258.2) * 227.1))) + 1;
}

}  // namespace sugarboat