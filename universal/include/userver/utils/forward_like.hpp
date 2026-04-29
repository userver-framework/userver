#pragma once

/// @file userver/utils/forward_like.hpp
/// @brief @copybrief utils::ForwardLike

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T, typename U>
struct ForwardLikeHelper;

template <typename T, typename U>
struct ForwardLikeHelper<T&, U&> : std::type_identity<U&> {};

template <typename T, typename U>
struct ForwardLikeHelper<const T&, U&> : std::type_identity<const U&> {};

template <typename T, typename U>
struct ForwardLikeHelper<T&&, U&> : std::type_identity<U&&> {};

template <typename T, typename U>
struct ForwardLikeHelper<const T&&, U&> : std::type_identity<const U&&> {};

}  // namespace impl

// Analogue of std::forward_like from c++23.
template <typename TOwner, typename TMember>
constexpr auto&& ForwardLike(TMember&& member) noexcept {
    using RType = impl::ForwardLikeHelper<TOwner&&, TMember&>::type;
    return static_cast<RType>(member);
}

}  // namespace utils

USERVER_NAMESPACE_END
