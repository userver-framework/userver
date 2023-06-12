#pragma once

#include <cstddef>
#include <string_view>
#include <type_traits>

#include <userver/logging/log_filepath.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

class SourceLocation {
 public:
  static SourceLocation Current(
      const char* filename = &__builtin_FILE()[std::integral_constant<
          size_t, logging::impl::PathBaseSize(__builtin_FILE())>::value],
      const char* function = __builtin_FUNCTION(),
      size_t line = __builtin_LINE());

  const char* file_name() const;
  const char* function_name() const;
  size_t line() const;

 private:
  SourceLocation(const char* filename, const char* function, size_t line);

  const char* file_name_;
  const char* function_name_;
  size_t line_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
