#pragma once

#include <random>

struct Mt19937BoundingInfo {
    uint64_t range_bitsize;
    uint64_t shift_value;
    uint64_t iterations0;
    uint64_t iterations1;
    uint64_t maxvalue0;
    uint64_t maxvalue1;
    uint32_t mask0;
    uint32_t mask1;
};

class Mt19937 {
    private:
        std::mt19937 generator;

        uint64_t mt_rand(uint64_t *params);
        uint64_t gen_rand(Mt19937BoundingInfo *info);

    public:
        Mt19937() {};
        ~Mt19937() {};

        void Seed(uint32_t value) { this->generator.seed(value); }
        uint64_t GetValue(uint64_t maxval);
};
