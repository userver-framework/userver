#include <utils/rand.hpp>

namespace utils::impl {

Random& GetRandom() {
  thread_local Random random;
  return random;
}

}  // namespace utils::impl
