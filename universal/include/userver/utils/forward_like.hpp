#pragma once

/// @file userver/utils/forward_like.hpp
/// @brief @copybrief utils::ForwardLike

#include <type_traits>
#include <utility>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Analogue of std::forward_like from C++23.
template <typename T, typename U>
USERVER_IMPL_NODEBUG_INLINE_FUNC inline auto&& ForwardLike(U&& x) noexcept {
    constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
    if constexpr (std::is_lvalue_reference_v<T&&>) {
        if constexpr (is_adding_const) {
            return std::as_const(x);
        } else {
            return x;
        }
    } else {
        if constexpr (is_adding_const) {
            return std::move(std::as_const(x));
        } else {
            return std::move(x);  // NOLINT(bugprone-move-forwarding-reference)
        }
    }
}

}  // namespace utils

USERVER_NAMESPACE_END
