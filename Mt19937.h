#pragma once

#include <random>

class Mt19937 {
    private:
        std::mt19937 generator;

    public:
        Mt19937() {};
        ~Mt19937() {};

        void Seed(uint32_t value) { this->generator.seed(value); }
        uint64_t GetValue(uint64_t maxval);
};
