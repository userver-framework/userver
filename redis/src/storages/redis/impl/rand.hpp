#pragma once

#include <algorithm>
#include <cstdint>
#include <random>

namespace utils {
namespace impl {

class Random {
 public:
  Random() : gen_(std::random_device()()) {}

  uint32_t operator()() { return gen_(); }

  std::mt19937& GetGenerator() { return gen_; }

 private:
  std::mt19937 gen_;
};

inline Random& GetRandom() {
  thread_local Random random;
  return random;
}

}  // namespace impl

inline uint32_t Rand() { return impl::GetRandom()(); }

template <class RandomIt>
void Shuffle(RandomIt first, RandomIt last) {
  return std::shuffle(first, last, impl::GetRandom().GetGenerator());
}

}  // namespace utils
