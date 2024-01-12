#include <userver/utils/uuid7.hpp>

#include <userver/utils/boost_uuid7.hpp>
#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

std::string GenerateUuidV7() {
  const auto val = GenerateBoostUuidV7();
  return encoding::ToHex(val.begin(), val.size());
}

}  // namespace utils::generators

USERVER_NAMESPACE_END
