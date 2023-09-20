#pragma once

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

template <typename ExceptionType>
bool IsSubtype(const std::exception& ex) noexcept {
  static_assert(
      std::is_base_of_v<std::exception, ExceptionType>,
      "Exception types not inherited from std::exception are not supported");
  if constexpr (std::is_same_v<ExceptionType, std::exception>) {
    return true;
  } else {
    return dynamic_cast<const ExceptionType*>(&ex) != nullptr;
  }
}

std::string AssertThrow(std::function<void()> statement,
                        std::string_view statement_text,
                        std::function<bool(const std::exception&)> type_checker,
                        const std::type_info& expected_type,
                        std::string_view message_substring);

std::string AssertNoThrow(std::function<void()> statement,
                          std::string_view statement_text);

}  // namespace utest::impl

USERVER_NAMESPACE_END
