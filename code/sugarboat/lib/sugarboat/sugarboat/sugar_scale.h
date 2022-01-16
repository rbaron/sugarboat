#ifndef _SUGARBOAT_SUGAR_SCALE_H_
#define _SUGARBOAT_SUGAR_SCALE_H_

namespace sugarboat {

// Represents the polynomial p(x) = a2*x^2 + a1*x + a0.
struct Coeffs {
  float a2;
  float a1;
  float a0;
};

float CalculateBrix(const Coeffs& coeffs, float tilt_angle);

float BrixToSG(float brix);

}  // namespace sugarboat
#endif  // _SUGARBOAT_SUGAR_SCALE_H_