#pragma once

namespace Market {

class BlackScholesProcess {
private:
    double spot_;       // S0
    double rate_;       // r (taux sans risque)
    double volatility_; // sigma
    double dividend_;   // q (dividende continu)

public:
    BlackScholesProcess(double spot, double rate, double volatility, double dividend = 0.0)
        : spot_(spot), rate_(rate), volatility_(volatility), dividend_(dividend) {}

    // Getters marqués nodiscard
    [[nodiscard]] double getSpot() const { return spot_; }
    [[nodiscard]] double getRate() const { return rate_; }
    [[nodiscard]] double getVolatility() const { return volatility_; }
    [[nodiscard]] double getDividend() const { return dividend_; }
};

} // namespace Market