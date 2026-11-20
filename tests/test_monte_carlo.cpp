#include <gtest/gtest.h>
#include "../include/Market/BlackScholesProcess.hpp"
#include "../include/Products/Option.hpp"
#include "../include/Products/Payoff.hpp"
#include "../include/Engines/AnalyticBSEngine.hpp"
#include "../include/Engines/MonteCarloEngine.hpp"
#include <cmath>
#include <memory>

namespace {

constexpr double kSpot = 100.0;
constexpr double kStrike = 100.0;
constexpr double kRate = 0.05;
constexpr double kVol = 0.20;
constexpr double kMaturity = 1.0;

// Down-and-out call, continuous monitoring (Merton / Reiner-Rubinstein), valid for K >= H, q = 0
double analyticDownAndOutCall(double S, double K, double H, double r, double v, double T) {
    const double sqrtT = std::sqrt(T);
    const double d1 = (std::log(S / K) + (r + 0.5 * v * v) * T) / (v * sqrtT);
    const double vanilla = S * Math::norm_cdf(d1) - K * std::exp(-r * T) * Math::norm_cdf(d1 - v * sqrtT);
    const double lambda = (r + 0.5 * v * v) / (v * v);
    const double y = std::log(H * H / (S * K)) / (v * sqrtT) + lambda * v * sqrtT;
    const double downAndIn = S * std::pow(H / S, 2.0 * lambda) * Math::norm_cdf(y)
        - K * std::exp(-r * T) * std::pow(H / S, 2.0 * lambda - 2.0) * Math::norm_cdf(y - v * sqrtT);
    return vanilla - downAndIn;
}

} // namespace

TEST(MonteCarloEngineTest, VanillaCallConvergesToBlackScholes) {
    Market::BlackScholesProcess process(kSpot, kRate, kVol);
    Products::VanillaOption call(std::make_unique<Products::PayoffCall>(kStrike), kMaturity);

    const double exact = Engines::AnalyticBSEngine::calculate(call, process, kStrike).price;
    const auto mc = Engines::MonteCarloEngine::calculate(call, process, 500'000);

    ASSERT_TRUE(mc.standardError.has_value());
    EXPECT_NEAR(mc.price, exact, 3.0 * mc.standardError.value());
}

TEST(MonteCarloEngineTest, ParallelEngineConvergesToBlackScholes) {
    Market::BlackScholesProcess process(kSpot, kRate, kVol);
    Products::VanillaOption put(std::make_unique<Products::PayoffPut>(kStrike), kMaturity);

    const double exact = Engines::AnalyticBSEngine::calculate(put, process, kStrike).price;
    const auto mc = Engines::MonteCarloEngine::calculateParallel(put, process, 500'000);

    ASSERT_TRUE(mc.standardError.has_value());
    EXPECT_NEAR(mc.price, exact, 3.0 * mc.standardError.value());
}

TEST(MonteCarloEngineTest, StandardErrorShrinksLikeOneOverSqrtN) {
    Market::BlackScholesProcess process(kSpot, kRate, kVol);
    Products::VanillaOption call(std::make_unique<Products::PayoffCall>(kStrike), kMaturity);

    const auto small = Engines::MonteCarloEngine::calculate(call, process, 10'000);
    const auto large = Engines::MonteCarloEngine::calculate(call, process, 1'000'000);

    // 100x more paths -> standard error about 10x smaller
    const double ratio = small.standardError.value() / large.standardError.value();
    EXPECT_NEAR(ratio, 10.0, 1.0);
}

TEST(MonteCarloEngineTest, SameSeedGivesSameResult) {
    Market::BlackScholesProcess process(kSpot, kRate, kVol);
    Products::VanillaOption call(std::make_unique<Products::PayoffCall>(kStrike), kMaturity);

    const auto a = Engines::MonteCarloEngine::calculate(call, process, 50'000, 1, 7);
    const auto b = Engines::MonteCarloEngine::calculate(call, process, 50'000, 1, 7);
    EXPECT_DOUBLE_EQ(a.price, b.price);
}

TEST(MonteCarloEngineTest, DownAndOutCallMatchesBroadieGlassermanKouCorrection) {
    Market::BlackScholesProcess process(kSpot, kRate, kVol);
    const double barrierLevel = 90.0;
    const size_t steps = 252;
    Products::BarrierOption barrier(std::make_unique<Products::PayoffCall>(kStrike), kMaturity, barrierLevel);
    Products::VanillaOption vanilla(std::make_unique<Products::PayoffCall>(kStrike), kMaturity);

    const auto mc = Engines::MonteCarloEngine::calculateParallel(barrier, process, 200'000, steps);
    const double vanillaPrice = Engines::AnalyticBSEngine::calculate(vanilla, process, kStrike).price;

    // A knock-out option is always worth less than the vanilla
    EXPECT_LT(mc.price, vanillaPrice);

    // Discrete daily monitoring: shift the barrier down by exp(-0.5826 * sigma * sqrt(dt))
    const double dt = kMaturity / static_cast<double>(steps);
    const double shiftedBarrier = barrierLevel * std::exp(-0.5826 * kVol * std::sqrt(dt));
    const double reference = analyticDownAndOutCall(kSpot, kStrike, shiftedBarrier, kRate, kVol, kMaturity);

    // 4 standard errors plus a small allowance for the approximation error of the correction
    EXPECT_NEAR(mc.price, reference, 4.0 * mc.standardError.value() + 0.02);
}
