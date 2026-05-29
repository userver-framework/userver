#pragma once

/// @file userver/utils/get_if.hpp
/// @brief @copybrief utils::GetIf
/// @ingroup userver_universal

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T>
concept IsPointerLike = requires(T& t) { t ? std::addressof(*std::declval<T&&>()) : nullptr; };

}  // namespace impl

template <typename Leaf>
constexpr auto* GetIf(Leaf&& leaf) {
    if constexpr (impl::IsPointerLike<Leaf>) {
        return leaf ? utils::GetIf(*std::forward<Leaf>(leaf)) : nullptr;
    } else {
        return std::addressof(std::forward<Leaf>(leaf));
    }
}

/// @brief Dereferences a chain of indirections and compositions,
/// returns nullptr if one of the chain elements is not set
///
/// @snippet universal/src/utils/get_if_test.cpp Sample Usage
template <typename Root, typename Head, typename... Tail>
constexpr auto* GetIf(Root&& root, Head&& head, Tail&&... tail) {
    if constexpr (impl::IsPointerLike<Root>) {
        return root ? utils::GetIf(*std::forward<Root>(root), std::forward<Head>(head), std::forward<Tail>(tail)...)
                    : nullptr;
    } else {
        return utils::GetIf(
            std::invoke(std::forward<Head>(head), std::forward<Root>(root)),
            std::forward<Tail>(tail)...
        );
    }
}

}  // namespace utils

USERVER_NAMESPACE_END
