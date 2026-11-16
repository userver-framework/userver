#pragma once

/// @file userver/utils/overloaded.hpp
/// @brief @copybrief utils::Overloaded

#include <type_traits>
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

namespace impl {

struct DeduceVisitResult final {};

}  // namespace impl

/// @brief Calls the matching handler for the active alternative of @a var.
/// @tparam R Return type. If omitted, it is deduced from the handlers. If `void`, the handler result is discarded.
/// @param var Variant to visit.
/// @param ts Handlers combined into @ref utils::Overloaded.
/// @returns The selected handler result, converted to `R` when specified; nothing for `void` results.
template <class R = impl::DeduceVisitResult, class... Args, class... Ts>
constexpr decltype(auto) Visit(const std::variant<Args...>& var, Ts&&... ts) {
    if constexpr (std::is_same_v<R, impl::DeduceVisitResult>) {
        return std::visit(Overloaded{std::forward<Ts>(ts)...}, var);
    } else {
        return std::visit<R>(Overloaded{std::forward<Ts>(ts)...}, var);
    }
}

/// @overload
template <class R = impl::DeduceVisitResult, class... Args, class... Ts>
constexpr decltype(auto) Visit(std::variant<Args...>& var, Ts&&... ts) {
    if constexpr (std::is_same_v<R, impl::DeduceVisitResult>) {
        return std::visit(Overloaded{std::forward<Ts>(ts)...}, var);
    } else {
        return std::visit<R>(Overloaded{std::forward<Ts>(ts)...}, var);
    }
}

/// @overload
template <class R = impl::DeduceVisitResult, class... Args, class... Ts>
constexpr decltype(auto) Visit(std::variant<Args...>&& var, Ts&&... ts) {
    if constexpr (std::is_same_v<R, impl::DeduceVisitResult>) {
        return std::visit(Overloaded{std::forward<Ts>(ts)...}, std::move(var));
    } else {
        return std::visit<R>(Overloaded{std::forward<Ts>(ts)...}, std::move(var));
    }
}

}  // namespace utils

USERVER_NAMESPACE_END
