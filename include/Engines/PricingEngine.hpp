#pragma once
#include <optional>

namespace Engines {

// Structure pour regrouper les résultats de pricing
struct PricingResults {
    double price;
    std::optional<double> delta;
    std::optional<double> gamma;
    std::optional<double> vega;
    std::optional<double> theta;
    std::optional<double> rho;
    
    // Pour le Monte Carlo plus tard
    std::optional<double> standardError; 
};

} // namespace Engines