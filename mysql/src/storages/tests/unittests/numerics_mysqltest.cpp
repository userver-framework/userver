#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

struct NonOptionalRow final {
  std::uint8_t uint8;
  std::int8_t int8;
  std::uint16_t uint16;
  std::int16_t int16;
  std::uint32_t uint32;
  std::int32_t int32;
  std::uint64_t uint64;
  std::int64_t int64;
};

struct OptionalRow final {};

}  // namespace

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
