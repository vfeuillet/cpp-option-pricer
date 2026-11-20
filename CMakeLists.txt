cmake_minimum_required(VERSION 3.15)
project(QuantPricer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

# Header-only pricing library
add_library(pricer INTERFACE)
target_include_directories(pricer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pricer INTERFACE Threads::Threads)

# Strict warnings on our own code only (not on third-party GoogleTest)
set(STRICT_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(STRICT_FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

# --- Benchmark / demo executable ---
add_executable(QuantPricer src/main.cpp)
target_link_libraries(QuantPricer PRIVATE pricer)
target_compile_options(QuantPricer PRIVATE ${STRICT_FLAGS})

# --- Unit tests (GoogleTest fetched at configure time) ---
option(BUILD_TESTS "Build the unit tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(UnitTests
        tests/test_payoff.cpp
        tests/test_analytic_bs.cpp
        tests/test_monte_carlo.cpp
    )
    target_link_libraries(UnitTests PRIVATE pricer GTest::gtest_main)
    target_compile_options(UnitTests PRIVATE ${STRICT_FLAGS})

    include(GoogleTest)
    gtest_discover_tests(UnitTests)
endif()
