#include <random>
#pragma once

inline std::mt19937 &randomEng() {
  static std::mt19937 gen(std::random_device{}());
  return gen;
}