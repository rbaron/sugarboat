#ifndef _SUGARBOAT_SUGAR_SCALE_H_
#define _SUGARBOAT_SUGAR_SCALE_H_

namespace sugarboat {

// Represents the polynomial p(x) = a2*x^2 + a1*x + a0.
// These hard-coded coefficients were obtained by calibrating a particular
// sugarboat against many solutions with known density. While these hard-coded
// coefficients are better than nothing, it's imperative to calibrate each
// sugarboat individually and update these in the config.
struct Coeffs {
  float a2 = 3.47e-3f;
  float a1 = 2.02e-1f;
  float a0 = -8.6f;
};

float CalculateBrix(const Coeffs& coeffs, float tilt_angle);

float BrixToSG(float brix);

}  // namespace sugarboat
#endif  // _SUGARBOAT_SUGAR_SCALE_H_