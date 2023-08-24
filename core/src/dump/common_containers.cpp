#include <userver/dump/common_containers.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump::impl {

[[noreturn]] void ThrowInvalidVariantIndex(const std::type_info& type,
                                           std::size_t index) {
  throw std::runtime_error(
      fmt::format("Invalid std::variant index in dump: type='{}', index={}",
                  compiler::GetTypeName(type), index));
}

}  // namespace dump::impl

USERVER_NAMESPACE_END
