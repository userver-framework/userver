#include "uuid4.hpp"

#include <boost/lockfree/stack.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>

#include <utils/encoding/hex.hpp>

namespace utils {
namespace generators {

namespace {

const size_t kMaxGeneratorBufferCount = 16;

class Uuid4Generator {
 public:
  using Uuid = boost::uuids::uuid;
  using RandomGenerator = boost::uuids::random_generator;

  ~Uuid4Generator() {
    RandomGenerator* gen = nullptr;
    while (stack_.pop(gen)) delete gen;
  }

  std::string Generate() {
    Uuid val = RandomGeneratorHolder(*this, Acquire())();

    return encoding::ToHex(val.begin(), val.end());
  }

 private:
  class RandomGeneratorHolder {
   public:
    RandomGeneratorHolder(Uuid4Generator& uuid_generator,
                          RandomGenerator* random_generator)
        : uuid_generator_(uuid_generator),
          random_generator_(random_generator) {}

    ~RandomGeneratorHolder() { uuid_generator_.Release(random_generator_); }

    Uuid operator()() const { return (*random_generator_)(); }

   private:
    Uuid4Generator& uuid_generator_;
    RandomGenerator* random_generator_;
  };

  RandomGenerator* Acquire() {
    RandomGenerator* gen = nullptr;
    if (stack_.pop(gen)) return gen;
    return new RandomGenerator();
  }

  void Release(RandomGenerator* gen) {
    if (gen == nullptr) return;
    if (stack_.bounded_push(gen)) return;
    delete gen;
  }

  using Stack = boost::lockfree::stack<
      RandomGenerator*, boost::lockfree::capacity<kMaxGeneratorBufferCount>>;
  Stack stack_;
};

}  // namespace

std::string GenerateUuid() {
  static Uuid4Generator generator;
  return generator.Generate();
}

}  // namespace generators
}  // namespace utils
