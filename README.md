# C++ Options Pricer

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![CMake](https://img.shields.io/badge/build-CMake-informational.svg)
![Tests](https://img.shields.io/badge/tests-GoogleTest-success.svg)

A header-only options pricer written from scratch in modern C++17. It prices European
options in closed form (Black-Scholes with all the Greeks) and by Monte Carlo, including a
path-dependent down-and-out barrier option, with a multi-threaded engine.

I built it as a portfolio project to go beyond academic Python scripts: the maths is
standard, the point was to write it the way production quant code is written (clear
ownership, no shared mutable state, strict compiler settings, and tests that check the
numbers against theory).

## Features

- **Analytic engine**: closed-form Black-Scholes-Merton price for European calls and puts,
  with a continuous dividend yield, and exact Greeks (Delta, Gamma, Vega, Theta, Rho).
- **Monte Carlo engine**: exact simulation of geometric Brownian motion,
  `S_{t+dt} = S_t · exp((r − q − σ²/2) dt + σ √dt · Z)`, with `std::mt19937_64`. Every
  price comes with its standard error, so a 95% confidence interval is
  `price ± 1.96 · SE`.
- **Path-dependent pricing**: the same engine prices a down-and-out barrier call on a
  discretised path (252 monitoring dates per year).
- **Multi-threading**: the parallel engine splits the paths across
  `std::thread::hardware_concurrency()` tasks launched with `std::async`. Each task has its
  own seeded generator and returns only partial sums (Σ payoff, Σ payoff²), which are added
  together at the end: a map-reduce with no locks and no shared mutable state.
- **Unit tests**: 11 GoogleTest cases, fetched automatically by CMake.

## Design choices

- **Stateless engines.** `AnalyticBSEngine` and `MonteCarloEngine` only expose static
  functions and keep no state, so they can be called from several threads without any
  synchronisation.
- **Product / payoff separation.** An `Option` (vanilla or barrier) decides *how* a path is
  turned into a cash flow through the virtual `cashFlow(path)`; a `Payoff` functor (call or
  put) decides *what* is paid at maturity. Any payoff can be combined with any exercise
  style, and the Monte Carlo engine prices every option through the same interface.
- **RAII and move semantics.** No raw `new` or `delete`. An option owns its payoff through a
  `std::unique_ptr`, which is moved into the constructor, so ownership is explicit and
  nothing can leak.
- **Explicit results.** `PricingResults` uses `std::optional` for the Greeks and the standard
  error: the analytic engine fills the Greeks, Monte Carlo fills the standard error, and the
  caller can see which values exist.
- **Zero-warning policy.** The project builds with `-Wall -Wextra -Wpedantic -Werror`.

## Validation

Parameters: `S = 100, K = 100, r = 5%, σ = 20%, T = 1 year`, no dividend.

| Product | Method | Price | Reference |
|---|---|---|---|
| European call | Black-Scholes closed form | 10.4506 | textbook value 10.4506 |
| European call | Monte Carlo, 1M paths (parallel) | ≈ 10.455 (SE ≈ 0.015) | within 1 SE of BS |
| Down-and-out call, H = 90 | Monte Carlo, 100k paths × 252 steps | ≈ 8.9–9.0 | 8.91 (see below) |

The continuous-monitoring closed form for the down-and-out call gives 8.67, but the
simulation only checks the barrier once a day, so the option is knocked out less often and
is worth more. Applying the Broadie-Glasserman-Kou correction (shifting the barrier to
`H · exp(−0.5826 σ √dt)`) gives 8.91, which is what the Monte Carlo engine finds.

The test suite checks these properties automatically:

| Test file | What it checks |
|---|---|
| `test_payoff.cpp` | call and put payoffs; a vanilla only depends on `S_T`; the barrier knocks out when touched and pays the vanilla otherwise |
| `test_analytic_bs.cpp` | put-call parity `C − P = S − K e^{−rT}`, `Δ_call − Δ_put = 1`, equal call and put Gamma |
| `test_monte_carlo.cpp` | sequential and parallel Monte Carlo within 3 SE of Black-Scholes; SE decreasing like `1/√N`; same seed gives the same price; barrier price below the vanilla and consistent with the BGK-corrected closed form |

## Performance

Measured on my laptop, Release build (`-O3`):

| Case | Sequential | Parallel |
|---|---|---|
| European call, 1,000,000 paths, 1 step | ~30–40 ms | ~7–10 ms (≈ 4.5× faster) |
| Down-and-out call, 100,000 paths × 252 steps (25.2M steps) | | ~120 ms |

The speed-up is below the number of cores because of thread start-up, the generation of the
Gaussian draws and the final reduction (Amdahl's law). Timings depend on the machine.

## Build and run

Requirements: a C++17 compiler (GCC, Clang or Apple Clang) and CMake ≥ 3.15. GoogleTest is
downloaded by CMake on the first configuration.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Unit tests
ctest --test-dir build --output-on-failure    # or ./build/UnitTests

# Pricing examples and benchmarks
./build/QuantPricer
```

To build without the tests (no download needed), add `-DBUILD_TESTS=OFF`.

## Project structure

```
include/
  Market/BlackScholesProcess.hpp    market data: spot, rate, volatility, dividend yield
  Products/Payoff.hpp               payoff functors (call, put)
  Products/Option.hpp               vanilla and down-and-out barrier options
  Engines/PricingEngine.hpp         PricingResults (price, Greeks, standard error)
  Engines/AnalyticBSEngine.hpp      closed-form Black-Scholes price and Greeks
  Engines/MonteCarloEngine.hpp      sequential and multi-threaded Monte Carlo
  Math/RandomGenerator.hpp          Gaussian draws (Mersenne Twister 64-bit)
  Math/Statistics.hpp               normal pdf and cdf
src/main.cpp                        examples and benchmarks
tests/                              GoogleTest unit tests
```

## Limitations and next steps

- Only one model (Black-Scholes, constant volatility). Natural extensions are Heston
  stochastic volatility and a local volatility surface.
- The Monte Carlo engine does not compute Greeks yet. Pathwise or likelihood-ratio
  estimators, or bumping with common random numbers, would be the next step.
- No variance reduction yet: antithetic variates and a control variate (the analytic vanilla
  price) would reduce the standard error at almost no cost.
- Only European exercise. American options would need a binomial tree, a PDE solver or
  Longstaff-Schwartz regression.
- The analytic engine identifies call and put with `dynamic_cast`; a type tag or a visitor
  would avoid RTTI as the number of payoffs grows.

## How I worked

I used an AI assistant as a pair programmer and mentor throughout this project. The goal was
not to generate code but to learn modern C++ faster: I used it to challenge my design
choices, understand why ownership and RAII matter, and compare my implementation with how
quant libraries are usually written. I can explain every part of the code.

## References

- Black, F. & Scholes, M. (1973). The pricing of options and corporate liabilities.
  *Journal of Political Economy*.
- Broadie, M., Glasserman, P. & Kou, S. (1997). A continuity correction for discrete barrier
  options. *Mathematical Finance*.
- Glasserman, P. (2003). *Monte Carlo Methods in Financial Engineering*. Springer.
- Hull, J. *Options, Futures, and Other Derivatives*.
