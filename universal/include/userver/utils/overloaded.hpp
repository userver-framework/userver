#pragma once

/// @file userver/utils/overloaded.hpp
/// @brief @copybrief utils::Overloaded

#include <variant>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal
///
/// @brief Utility to define std::variant visitors in a simple way
template <class... Ts>
struct Overloaded : Ts... {  // NOLINT(fuchsia-multiple-inheritance)
  using Ts::operator()...;
};

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template <class... Args, class... Ts>
constexpr decltype(auto) Visit(const std::variant<Args...>& var, Ts&&... ts) {
  return std::visit(Overloaded{std::forward<Ts>(ts)...}, var);
}

template <class... Args, class... Ts>
constexpr decltype(auto) Visit(std::variant<Args...>& var, Ts&&... ts) {
  return std::visit(Overloaded{std::forward<Ts>(ts)...}, var);
}

template <class... Args, class... Ts>
constexpr decltype(auto) Visit(std::variant<Args...>&& var, Ts&&... ts) {
  return std::visit(Overloaded{std::forward<Ts>(ts)...}, std::move(var));
}

}  // namespace utils

USERVER_NAMESPACE_END
