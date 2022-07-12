#include "Mt19937.h"

uint64_t Mt19937::GetValue(uint64_t maxval) {
  std::uniform_int_distribution<uint64_t> distribution(0,maxval);
  return distribution(this->generator);
}

