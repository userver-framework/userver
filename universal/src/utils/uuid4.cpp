#include <userver/utils/uuid4.hpp>

#include <userver/utils/boost_uuid4.hpp>
#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

std::string GenerateUuid() {
  const auto val = GenerateBoostUuid();
  return encoding::ToHex(val.begin(), val.size());
}

}  // namespace utils::generators

USERVER_NAMESPACE_END
