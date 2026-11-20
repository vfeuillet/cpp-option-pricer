#include "../include/Market/BlackScholesProcess.hpp"
#include "../include/Products/Option.hpp"
#include "../include/Products/Payoff.hpp"
#include "../include/Engines/AnalyticBSEngine.hpp"
#include "../include/Engines/MonteCarloEngine.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

int main() {
    // 1. Définition des paramètres
    double spot = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double volatility = 0.20;
    double maturity = 1.0;

    Market::BlackScholesProcess process(spot, rate, volatility);
    auto callPayoff = std::make_unique<Products::PayoffCall>(strike);
    Products::VanillaOption callOption(std::move(callPayoff), maturity);

    // 2. Pricing Analytique (La vérité absolue)
    auto analyticResults = Engines::AnalyticBSEngine::calculate(callOption, process, strike);

    // 3. Pricing Monte-Carlo avec Benchmark (Séquentiel vs Parallèle)
    size_t numSimulations = 1'000'000; 
    
    // --- Séquentiel ---
    auto start_seq = std::chrono::high_resolution_clock::now();
    auto mcResultsSeq = Engines::MonteCarloEngine::calculate(callOption, process, numSimulations);
    auto end_seq = std::chrono::high_resolution_clock::now();
    auto duration_seq = std::chrono::duration_cast<std::chrono::milliseconds>(end_seq - start_seq);

    // --- Parallèle ---
    auto start_par = std::chrono::high_resolution_clock::now();
    auto mcResultsPar = Engines::MonteCarloEngine::calculateParallel(callOption, process, numSimulations);
    auto end_par = std::chrono::high_resolution_clock::now();
    auto duration_par = std::chrono::duration_cast<std::chrono::milliseconds>(end_par - start_par);

    // 4. Affichage des résultats
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== COMPARAISON ANALYTIQUE vs MONTE-CARLO ===\n\n";
    std::cout << "Prix Analytique (BS)       : " << analyticResults.price << "\n";
    std::cout << "Prix MC Sequentiel         : " << mcResultsSeq.price << "\n";
    std::cout << "Prix MC Parallele          : " << mcResultsPar.price << "\n\n";

    std::cout << "=== PERFORMANCE (" << numSimulations << " trajectoires) ===\n";
    std::cout << "Temps d'execution (Seq)    : " << duration_seq.count() << " ms\n";
    std::cout << "Temps d'execution (Par)    : " << duration_par.count() << " ms\n";
    
    if (duration_par.count() > 0) {
        double speedup = static_cast<double>(duration_seq.count()) / duration_par.count();
        std::cout << "Facteur d'acceleration     : x" << std::setprecision(2) << speedup << "\n";
    }

    // 5. EXTENSION : Pricing d'une Option à Barrière (Down-and-Out)
    std::cout << "\n=== PRICING OPTION BARRIERE (DOWN-AND-OUT) ===\n";
    double barrierLevel = 90.0; // Si le spot tombe à 90€, l'option est morte
    
    // On crée l'option Barrière Call (On doit ré-instancier un Payoff car l'autre a été 'moved')
    auto barrierPayoff = std::make_unique<Products::PayoffCall>(strike);
    Products::BarrierOption barrierOption(std::move(barrierPayoff), maturity, barrierLevel);

    // On price avec 252 pas de temps (le nombre de jours de trading dans une année)
    size_t numSteps = 252;
    // On réduit à 100 000 trajectoires car (100k paths * 252 steps) = 25 millions de calculs !
    size_t barrierSims = 100'000; 
    
    auto start_barrier = std::chrono::high_resolution_clock::now();
    auto barrierResults = Engines::MonteCarloEngine::calculateParallel(barrierOption, process, barrierSims, numSteps);
    auto end_barrier = std::chrono::high_resolution_clock::now();
    auto duration_barrier = std::chrono::duration_cast<std::chrono::milliseconds>(end_barrier - start_barrier);

    std::cout << "Type                       : Down-and-Out Call\n";
    std::cout << "Niveau de la barriere      : " << barrierLevel << " EUR\n";
    std::cout << "Prix de l'Option Vanille   : " << mcResultsPar.price << " (sans risque de desactivation)\n";
    std::cout << "Prix de l'Option Barriere  : " << barrierResults.price << "\n";
    std::cout << "Temps de calcul (" << numSteps << " steps) : " << duration_barrier.count() << " ms\n";

    return 0;
}