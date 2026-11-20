#pragma once
#include <cmath>

namespace Math {

// Constante pré-calculée pour optimiser (1 / sqrt(2 * PI))
constexpr double INV_SQRT_2PI = 0.3989422804014327;
// Constante de <cmath> : M_SQRT1_2 = 1 / sqrt(2)

// Probability Density Function (PDF) de la loi Normale N(0,1)
[[nodiscard]] inline double norm_pdf(double x) {
    return INV_SQRT_2PI * std::exp(-0.5 * x * x);
}

// Cumulative Distribution Function (CDF) de la loi Normale N(0,1)
[[nodiscard]] inline double norm_cdf(double x) {
    // std::erfc est plus stable numériquement que std::erf pour les grandes valeurs
    return 0.5 * std::erfc(-x * M_SQRT1_2);
}

} // namespace Math