#include "sugarboat/sugar_scale.h"

#include <math.h>

namespace sugarboat {

float CalculateSG(const Coeffs& coeffs, float tilt_angle) {
  return coeffs.a2 * tilt_angle * tilt_angle + coeffs.a1 * tilt_angle +
         coeffs.a0;
}

// https://en.wikipedia.org/wiki/Brix
// float BrixToSG(float brix) {
// 	return 182.4601 * powf(brix, 3) +
// }

}  // namespace sugarboat