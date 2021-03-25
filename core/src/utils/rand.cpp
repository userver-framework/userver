#include <utils/rand.hpp>

namespace utils {

namespace {

class RandomImpl final : public RandomBase {
 public:
  RandomImpl() : gen_(std::random_device()()) {}

  uint32_t operator()() override { return gen_(); }

 private:
  std::mt19937 gen_;
};

}  // namespace

RandomBase& DefaultRandom() {
  thread_local RandomImpl random;
  return random;
}

uint32_t Rand() {
  return std::uniform_int_distribution<uint32_t>{0}(DefaultRandom());
}

}  // namespace utils
