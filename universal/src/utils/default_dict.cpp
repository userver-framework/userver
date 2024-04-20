#include <userver/utils/default_dict.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

[[noreturn]] void ThrowNoValueException(std::string_view dict_name,
                                        std::string_view key) {
  throw std::runtime_error(
      fmt::format("no value for '{}' in dict '{}'", key, dict_name));
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
