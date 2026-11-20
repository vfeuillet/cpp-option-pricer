#pragma once
#include "PricingEngine.hpp"
#include "../Products/Option.hpp"
#include "../Market/BlackScholesProcess.hpp"
#include "../Math/Statistics.hpp"
#include <cmath>
#include <stdexcept>

namespace Engines {

class AnalyticBSEngine {
public:
    // Méthode de calcul stateless
    [[nodiscard]] static PricingResults calculate(
        const Products::VanillaOption& option, 
        const Market::BlackScholesProcess& process,
        double strike) // Pour simplifier l'extraction du strike pour l'instant
    {
        double S = process.getSpot();
        double K = strike;
        double r = process.getRate();
        double q = process.getDividend();
        double v = process.getVolatility();
        double T = option.getMaturity();

        if (T <= 0.0) {
            throw std::invalid_argument("La maturité doit être strictement positive.");
        }

        double d1 = (std::log(S / K) + (r - q + 0.5 * v * v) * T) / (v * std::sqrt(T));
        double d2 = d1 - v * std::sqrt(T);

        double pdf_d1 = Math::norm_pdf(d1);
        double cdf_d1 = Math::norm_cdf(d1);
        double cdf_d2 = Math::norm_cdf(d2);

        // Facteurs d'escompte
        double exp_qT = std::exp(-q * T);
        double exp_rT = std::exp(-r * T);

        PricingResults results;

        // Détection Call ou Put (RTTI)
        const auto* payoff = option.getPayoff(); // À ajouter dans Option.hpp !
        
        if (dynamic_cast<const Products::PayoffCall*>(payoff)) {
            // -- CALL --
            results.price = S * exp_qT * cdf_d1 - K * exp_rT * cdf_d2;
            results.delta = exp_qT * cdf_d1;
            results.gamma = (exp_qT * pdf_d1) / (S * v * std::sqrt(T));
            results.vega  = S * exp_qT * pdf_d1 * std::sqrt(T);
            results.theta = -(S * exp_qT * pdf_d1 * v) / (2 * std::sqrt(T)) 
                            - r * K * exp_rT * cdf_d2 
                            + q * S * exp_qT * cdf_d1;
            results.rho   = K * T * exp_rT * cdf_d2;
        } 
        else if (dynamic_cast<const Products::PayoffPut*>(payoff)) {
            // -- PUT --
            double cdf_minus_d1 = Math::norm_cdf(-d1);
            double cdf_minus_d2 = Math::norm_cdf(-d2);

            results.price = K * exp_rT * cdf_minus_d2 - S * exp_qT * cdf_minus_d1;
            results.delta = exp_qT * (cdf_d1 - 1.0);
            results.gamma = (exp_qT * pdf_d1) / (S * v * std::sqrt(T)); // Identique au Call
            results.vega  = S * exp_qT * pdf_d1 * std::sqrt(T);         // Identique au Call
            results.theta = -(S * exp_qT * pdf_d1 * v) / (2 * std::sqrt(T)) 
                            + r * K * exp_rT * cdf_minus_d2 
                            - q * S * exp_qT * cdf_minus_d1;
            results.rho   = -K * T * exp_rT * cdf_minus_d2;
        } 
        else {
            throw std::runtime_error("Payoff non supporté pour AnalyticBSEngine (seul Call/Put vanille sont acceptés).");
        }

        return results;
    }
};

} // namespace Engines