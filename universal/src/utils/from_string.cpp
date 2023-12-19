#include <userver/utils/from_string.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

[[noreturn]] void ThrowFromStringException(std::string_view message,
                                           std::string_view input,
                                           std::type_index resultType) {
  throw std::runtime_error(fmt::format(
      R"(utils::FromString error: "{}" while converting "{}" to {})", message,
      input, compiler::GetTypeName(resultType)));
}

}  // namespace impl

std::int64_t FromHexString(const std::string& str) {
  std::int64_t result{};
  try {
    result = std::stoll(str, nullptr, 16);
  } catch (std::logic_error& ex) {
    throw std::runtime_error(fmt::format(
        R"(utils::FromHexString error: Error while converting "{}" to std::int64_t)",
        str));
  }

  return result;
}

}  // namespace utils

USERVER_NAMESPACE_END
