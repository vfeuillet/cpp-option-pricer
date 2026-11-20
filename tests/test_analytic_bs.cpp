#include <gtest/gtest.h>
#include "../include/Market/BlackScholesProcess.hpp"
#include "../include/Products/Option.hpp"
#include "../include/Products/Payoff.hpp"
#include "../include/Engines/AnalyticBSEngine.hpp"
#include <cmath>
#include <memory>

// Un test GTest est défini par la macro TEST(NomDeLaSuite, NomDuTest)
TEST(AnalyticBSEngineTest, CallPutParity) {
    // 1. Arrange (Mise en place des données)
    double spot = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double vol = 0.20;
    double maturity = 1.0;

    Market::BlackScholesProcess process(spot, rate, vol);
    Products::VanillaOption callOption(std::make_unique<Products::PayoffCall>(strike), maturity);
    Products::VanillaOption putOption(std::make_unique<Products::PayoffPut>(strike), maturity);

    // 2. Act (L'action qu'on veut tester)
    auto callResults = Engines::AnalyticBSEngine::calculate(callOption, process, strike);
    auto putResults  = Engines::AnalyticBSEngine::calculate(putOption, process, strike);

    // 3. Assert (Les vérifications)
    double callMinusPut = callResults.price - putResults.price;
    double impliedForward = spot - strike * std::exp(-rate * maturity);

    // On utilise EXPECT_NEAR pour comparer des doubles (jamais ==)
    EXPECT_NEAR(callMinusPut, impliedForward, 1e-10);
    
    // On vérifie aussi que les Grecques sont bien calculées et respectent la théorie (Delta Call - Delta Put = 1)
    ASSERT_TRUE(callResults.delta.has_value() && putResults.delta.has_value());
    EXPECT_NEAR(callResults.delta.value() - putResults.delta.value(), 1.0, 1e-10);
    
    // Le Gamma d'un Call et d'un Put au même strike doit être identique
    ASSERT_TRUE(callResults.gamma.has_value() && putResults.gamma.has_value());
    EXPECT_NEAR(callResults.gamma.value(), putResults.gamma.value(), 1e-10);
}