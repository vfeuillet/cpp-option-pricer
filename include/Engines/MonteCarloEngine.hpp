#pragma once
#include "PricingEngine.hpp"
#include "../Products/Option.hpp"
#include "../Market/BlackScholesProcess.hpp"
#include "../Math/RandomGenerator.hpp"
#include <cmath>
#include <vector>
#include <future>
#include <thread>

namespace Engines {

class MonteCarloEngine {
public:
    // Moteur Séquentiel
    [[nodiscard]] static PricingResults calculate(
        const Products::Option& option, 
        const Market::BlackScholesProcess& process,
        size_t numSimulations,
        size_t numSteps = 1,
        unsigned int seed = 42) 
    {
        return runSimulations(option, process, numSimulations, numSteps, seed);
    }

    // Moteur Parallèle
    [[nodiscard]] static PricingResults calculateParallel(
        const Products::Option& option, 
        const Market::BlackScholesProcess& process,
        size_t numSimulations,
        size_t numSteps = 1,
        unsigned int baseSeed = 42) 
    {
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;

        size_t pathsPerThread = numSimulations / numThreads;
        size_t remainingPaths = numSimulations % numThreads;

        struct ChunkResult { double sum; double sumSq; };

        auto exactWorker = [&](size_t paths, unsigned int localSeed) -> ChunkResult {
            double S0 = process.getSpot();
            double r = process.getRate();
            double q = process.getDividend();
            double v = process.getVolatility();
            double T = option.getMaturity();
            double dt = T / numSteps;
            double drift = (r - q - 0.5 * v * v) * dt;
            double volTerm = v * std::sqrt(dt);

            auto gaussians = Math::RandomGenerator::generateStandardNormal(paths * numSteps, localSeed);
            
            double localSum = 0.0;
            double localSumSq = 0.0;
            std::vector<double> path(numSteps);

            for (size_t i = 0; i < paths; ++i) {
                double St = S0;
                for (size_t t = 0; t < numSteps; ++t) {
                    St *= std::exp(drift + volTerm * gaussians[i * numSteps + t]);
                    path[t] = St;
                }
                double payoff = option.cashFlow(path);
                localSum += payoff;
                localSumSq += payoff * payoff;
            }
            return {localSum, localSumSq};
        };

        std::vector<std::future<ChunkResult>> futures;
        for (unsigned int i = 0; i < numThreads; ++i) {
            size_t paths = pathsPerThread + (i == 0 ? remainingPaths : 0);
            futures.push_back(std::async(std::launch::async, exactWorker, paths, baseSeed + i));
        }

        double totalSum = 0.0;
        double totalSumSq = 0.0;
        for (auto& f : futures) {
            ChunkResult res = f.get();
            totalSum += res.sum;
            totalSumSq += res.sumSq;
        }

        double exp_rT = std::exp(-process.getRate() * option.getMaturity());
        double meanPayoff = totalSum / numSimulations;
        double variance = (totalSumSq - (totalSum * totalSum) / numSimulations) / (numSimulations - 1);
        
        PricingResults results;
        results.price = meanPayoff * exp_rT;
        results.standardError = (std::sqrt(variance) / std::sqrt(numSimulations)) * exp_rT;
        return results;
    }

private:
    static PricingResults runSimulations(const Products::Option& option, const Market::BlackScholesProcess& process, size_t paths, size_t numSteps, unsigned int seed) {
        double S0 = process.getSpot();
        double r = process.getRate();
        double q = process.getDividend();
        double v = process.getVolatility();
        double T = option.getMaturity();
        
        double dt = T / numSteps;
        double drift = (r - q - 0.5 * v * v) * dt;
        double volTerm = v * std::sqrt(dt);

        auto gaussians = Math::RandomGenerator::generateStandardNormal(paths * numSteps, seed);
        
        double sum = 0.0, sumSq = 0.0;
        std::vector<double> path(numSteps);

        for (size_t i = 0; i < paths; ++i) {
            double St = S0;
            for (size_t t = 0; t < numSteps; ++t) {
                St *= std::exp(drift + volTerm * gaussians[i * numSteps + t]);
                path[t] = St;
            }
            double payoff = option.cashFlow(path);
            sum += payoff;
            sumSq += payoff * payoff;
        }

        double exp_rT = std::exp(-r * T);
        double mean = sum / paths;
        double var = (sumSq - (sum * sum) / paths) / (paths - 1);
        
        return { mean * exp_rT, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, (std::sqrt(var) / std::sqrt(paths)) * exp_rT };
    }
};

} // namespace Engines