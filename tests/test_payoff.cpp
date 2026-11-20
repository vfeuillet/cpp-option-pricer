#include <gtest/gtest.h>
#include "../include/Products/Option.hpp"
#include "../include/Products/Payoff.hpp"
#include <memory>
#include <vector>

TEST(PayoffTest, CallPayoff) {
    Products::PayoffCall call(100.0);
    EXPECT_DOUBLE_EQ(call(120.0), 20.0);
    EXPECT_DOUBLE_EQ(call(100.0), 0.0);
    EXPECT_DOUBLE_EQ(call(80.0), 0.0);
}

TEST(PayoffTest, PutPayoff) {
    Products::PayoffPut put(100.0);
    EXPECT_DOUBLE_EQ(put(80.0), 20.0);
    EXPECT_DOUBLE_EQ(put(100.0), 0.0);
    EXPECT_DOUBLE_EQ(put(120.0), 0.0);
}

TEST(OptionTest, VanillaUsesOnlyTerminalSpot) {
    Products::VanillaOption call(std::make_unique<Products::PayoffCall>(100.0), 1.0);
    // The path dips far below the strike, but only S_T matters for a European option
    const std::vector<double> path{50.0, 70.0, 110.0};
    EXPECT_DOUBLE_EQ(call.cashFlow(path), 10.0);
}

TEST(OptionTest, DownAndOutIsKnockedOutWhenBarrierIsHit) {
    Products::BarrierOption barrier(std::make_unique<Products::PayoffCall>(100.0), 1.0, 90.0);
    const std::vector<double> touched{95.0, 89.9, 120.0};
    EXPECT_DOUBLE_EQ(barrier.cashFlow(touched), 0.0);
}

TEST(OptionTest, DownAndOutPaysVanillaWhenBarrierIsNotHit) {
    Products::BarrierOption barrier(std::make_unique<Products::PayoffCall>(100.0), 1.0, 90.0);
    const std::vector<double> alive{95.0, 91.0, 120.0};
    EXPECT_DOUBLE_EQ(barrier.cashFlow(alive), 20.0);
}
