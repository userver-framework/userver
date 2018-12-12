#include <utils/uuid4.hpp>

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

  std::string Generate() {
    static thread_local RandomGenerator generator;
    Uuid val = generator();

    return encoding::ToHex(val.begin(), val.end());
  }
};

}  // namespace

std::string GenerateUuid() {
  static Uuid4Generator generator;
  return generator.Generate();
}

}  // namespace generators
}  // namespace utils
