#pragma once

/// @file userver/utils/forward_like.hpp
/// @brief @copybrief utils::ForwardLike

#include <type_traits>

#include <userver/compiler/impl/nodebug.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Analogue of std::forward_like from C++23.
template <typename T, typename U>
USERVER_IMPL_NODEBUG_INLINE_FUNC inline constexpr auto&& ForwardLike(U&& x) noexcept {
    constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;

    using URef = std::remove_reference_t<U>;
    using V = std::conditional_t<is_adding_const, std::add_const_t<URef>, URef>;

    if constexpr (std::is_lvalue_reference_v<T&&>) {
        return static_cast<V&>(x);
    } else {
        return static_cast<V&&>(x);
    }
}

}  // namespace utils
USERVER_NAMESPACE_END
