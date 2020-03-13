#include <utils/rand.hpp>

namespace utils {
namespace impl {

Random& GetRandom() {
  thread_local Random random;
  return random;
}

}  // namespace impl
}  // namespace utils
