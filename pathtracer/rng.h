#pragma once

#define SEED 1

// linear congruential generator
struct lcg {
    unsigned int state;
    const unsigned int a = 1664525;
    const unsigned int c = 1013904223;
    const unsigned long long m = 4294967296ull;

    lcg(unsigned int seed) : state(seed) {}

    unsigned int operator()() {
        state = (a * state + c) % m;
        return state;
    }
    float rand01() {
        return static_cast<float>((*this)()) / m;
    }
    void seed(unsigned int seed) {
        state = seed;
    }

    // define min and max to work with std::uniform_real_distribution
    unsigned long long max() const {
        return m;
    }
    unsigned long long min() const {
        return 0;
    }
} rng(SEED);
