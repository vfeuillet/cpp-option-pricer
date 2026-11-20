#pragma once
#include <random>
#include <vector>

namespace Math {

class RandomGenerator {
public:
    // Génère un vecteur de N tirages gaussiens N(0,1)
    [[nodiscard]] static std::vector<double> generateStandardNormal(size_t numSimulations, unsigned int seed = 42) {
        std::vector<double> randoms(numSimulations);
        
        // mt19937_64 est le Mersenne Twister optimisé pour les processeurs 64 bits
        std::mt19937_64 rng(seed);
        
        // Transforme les nombres uniformes du Mersenne Twister en distribution Normale
        std::normal_distribution<double> dist(0.0, 1.0);

        for (size_t i = 0; i < numSimulations; ++i) {
            randoms[i] = dist(rng);
        }
        
        return randoms;
    }
};

} // namespace Math