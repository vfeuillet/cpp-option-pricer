#pragma once
#include <algorithm> // pour std::max

namespace Products {

class Payoff {
public:
    // Destructeur virtuel : crucial !
    virtual ~Payoff() = default;

    // Surcharge de l'opérateur () : rend la classe "Callable" (Foncteur)
    [[nodiscard]] virtual double operator()(double spot) const = 0;
};

// Implémentation du Call
class PayoffCall : public Payoff {
private:
    double strike_;
public:
    explicit PayoffCall(double strike) : strike_(strike) {}
    
    [[nodiscard]] double operator()(double spot) const override {
        return std::max(spot - strike_, 0.0);
    }
};

// Implémentation du Put
class PayoffPut : public Payoff {
private:
    double strike_;
public:
    explicit PayoffPut(double strike) : strike_(strike) {}
    
    [[nodiscard]] double operator()(double spot) const override {
        return std::max(strike_ - spot, 0.0);
    }
};

} // namespace Products