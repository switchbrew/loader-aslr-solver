#include "Mt19937.h"

uint64_t Mt19937::GetValue(uint64_t maxval) {
  uint64_t params[2] = {0, maxval};

  return mt_rand(params);
}

uint64_t Mt19937::mt_rand(uint64_t *params) {
    uint64_t rng_maxval;
    uint32_t mask0, mask1;
    uint64_t bitsize, tmp;
    Mt19937BoundingInfo info;

    rng_maxval = params[1] - params[0];
    if (rng_maxval == 0) return params[0];

    rng_maxval++;
    if (rng_maxval == 0) {
        info.range_bitsize = 0x40;
        info.iterations1 = 2;
        info.maxvalue0 = 0x100000000;
        info.shift_value = 0x20;
        info.iterations0 = 2;
        info.maxvalue1 = 0;
        info.mask0 = 0xffffffff;
        info.mask1 = 0xffffffff;
        return gen_rand(&info);
    }

    tmp = __builtin_clzl(rng_maxval);
    if ((rng_maxval << tmp) & 0x7fffffffffffffff) bitsize = 0x40;
    else bitsize = 0x3f;
    info.range_bitsize = bitsize - tmp;
    info.iterations0 = info.range_bitsize >> 5;
    if (info.range_bitsize & 0x1f) info.iterations0++;
    info.shift_value = info.range_bitsize / info.iterations0;
    if (info.shift_value < 0x40) info.maxvalue0 = (0xffffffffffffffff << info.shift_value) & 0x100000000;
    else info.maxvalue0 = 0;

    if (info.maxvalue0 / info.iterations0 < 0x100000000 - info.maxvalue0) {
        info.iterations0++;
        info.shift_value = info.range_bitsize / info.iterations0;
        if (info.shift_value > 0x3f) {
            info.iterations1 = info.iterations0 + (info.range_bitsize % info.iterations0);
            info.maxvalue0 = 0;
            info.maxvalue1 = 0;
            goto mt_rand_l0;
        }
        info.maxvalue0 = (0xffffffffffffffff << info.shift_value) & 0x100000000;
    }

    info.iterations1 = info.iterations0 + (info.range_bitsize % info.iterations0);
    if (info.shift_value < 0x3f) info.maxvalue1 = (0xffffffffffffffff << (info.shift_value + 1)) & 0x100000000;
    else info.maxvalue1 = 0;

mt_rand_l0:
    if (info.shift_value) mask0 = 0xffffffff >> (-(uint32_t)info.shift_value & 0x1f);
    else mask0 = 0;
    if (info.shift_value < 0x1f) mask1 = 0xffffffff >> (0x1f - info.shift_value);
    else mask1 = 0xffffffff;
    info.mask0 = mask0;
    info.mask1 = mask1;

    do {
        tmp = gen_rand(&info);
    } while (rng_maxval <= tmp);

    return params[0] + tmp;
}

uint64_t Mt19937::gen_rand(Mt19937BoundingInfo *info) {
    uint32_t rngout;
    uint64_t outval=0, tmpshift, i=0;

    if (info->iterations1) {
        for (i=0; i<info->iterations1; i++) {
            do {
                rngout = this->generator();
            } while (info->maxvalue0 <= (uint64_t)rngout);

            if (info->shift_value < 0x40) tmpshift = outval << (info->shift_value & 0x3f);
            else tmpshift = 0;
            outval = tmpshift + (uint64_t)(info->mask0 & rngout);
        }
    }

    for (; i<info->iterations0; i++) {
        do {
            rngout = this->generator();
        } while (info->maxvalue1 <= (uint64_t)rngout);

        if (info->shift_value < 0x3f) tmpshift = outval << ((info->shift_value + 1) & 0x3f);
        else tmpshift = 0;
        outval = tmpshift + (uint64_t)(info->mask1 & rngout);
    }
    return outval;
}

