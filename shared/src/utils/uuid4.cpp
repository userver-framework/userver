#include <utils/uuid4.hpp>

#include <utils/boost_uuid4.hpp>
#include <utils/encoding/hex.hpp>

namespace utils::generators {

std::string GenerateUuid() {
  const auto val = GenerateBoostUuid();
  return encoding::ToHex(val.begin(), val.end());
}

}  // namespace utils::generators
