#pragma once
#include <cstdint>
#include <random>
inline std::mt19937 &randomEng() {
  static std::mt19937 gen(std::random_device{}());
  return gen;
}
inline void setSeed(std::uint32_t seed_) { randomEng().seed(seed_); }
inline void resetSeed() { randomEng().seed(std::random_device{}()); }