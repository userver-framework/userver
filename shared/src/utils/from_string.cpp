#include <utils/from_string.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <compiler/demangle.hpp>

namespace utils::impl {

[[noreturn]] void ThrowFromStringException(std::string_view message,
                                           std::string_view input,
                                           std::type_index resultType) {
  throw std::runtime_error(fmt::format(
      R"(utils::FromString error: "{}" while converting "{}" to {})", message,
      input, compiler::GetTypeName(resultType)));
}

}  // namespace utils::impl
