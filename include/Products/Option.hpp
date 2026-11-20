#pragma once
#include "Payoff.hpp"
#include <memory>
#include <vector>

namespace Products {

class Option {
protected:
    std::unique_ptr<Payoff> payoff_;
    double maturity_; // T

public:
    // Constructeur prenant possession du Payoff via move semantics
    Option(std::unique_ptr<Payoff> payoff, double maturity)
        : payoff_(std::move(payoff)), maturity_(maturity) {}

    virtual ~Option() = default;

    [[nodiscard]] double getMaturity() const { return maturity_; }

    // Méthode clé pour le Monte Carlo (path-dependent)
    [[nodiscard]] virtual double cashFlow(const std::vector<double>& path) const = 0;

    [[nodiscard]] const Payoff* getPayoff() const { return payoff_.get(); }
};

class VanillaOption : public Option {
public:
    // Héritage du constructeur de base
    VanillaOption(std::unique_ptr<Payoff> payoff, double maturity)
        : Option(std::move(payoff), maturity) {}

    // Pour une vanille, seul le dernier point de la trajectoire (spot à maturité) compte
    [[nodiscard]] double cashFlow(const std::vector<double>& path) const override {
        // (*payoff_) appelle l'operator() qu'on a défini plus tôt
        return (*payoff_)(path.back()); 
    }
};

class BarrierOption : public Option {
private:
    double barrier_;

public:
    // Option Down-and-Out
    BarrierOption(std::unique_ptr<Payoff> payoff, double maturity, double barrier)
        : Option(std::move(payoff), maturity), barrier_(barrier) {}

    // Implémentation du polymorphisme : la logique métier de la barrière
    [[nodiscard]] double cashFlow(const std::vector<double>& path) const override {
        // On parcourt toute la trajectoire
        for (double spot_t : path) {
            if (spot_t <= barrier_) {
                return 0.0; // La barrière a été touchée, l'option est morte
            }
        }
        // Si la barrière n'a pas été touchée, on paie le payoff normal à maturité
        return (*payoff_)(path.back());
    }
};
} // namespace Products