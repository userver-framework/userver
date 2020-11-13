#pragma once

#include <algorithm>
#include <cstdint>
#include <random>

namespace utils {

class RandomBase {
 public:
  virtual uint32_t operator()() = 0;
};

namespace impl {

class Random final : public RandomBase {
 public:
  Random() : gen_(std::random_device()()) {}

  uint32_t operator()() override { return gen_(); }

  std::mt19937& GetGenerator() { return gen_; }

 private:
  std::mt19937 gen_;
};

Random& GetRandom();

}  // namespace impl

inline uint32_t Rand() { return impl::GetRandom()(); }

template <class RandomIt>
void Shuffle(RandomIt first, RandomIt last) {
  return std::shuffle(first, last, impl::GetRandom().GetGenerator());
}

}  // namespace utils
