#pragma once

/// @file userver/utils/span.hpp
/// @brief @copybrief utils::span

#include <concepts>
#include <cstddef>
#include <iterator>
#include <span>
#include <type_traits>

// TODO remove extra include
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// A polyfill for std::span with some of the newer features enabled.
template <typename T>
class span : public std::span<T> {  // NOLINT(readability-identifier-naming)
public:
    // std::span gains this alias only in C++23.
    using const_iterator = typename std::span<T>::iterator;

    using std::span<T>::span;

    constexpr explicit(false) span(std::span<T> s) noexcept : std::span<T>(s) {}

    // Allows converting utils::span<U> to utils::span<T>, following std::span.
    template <typename U>
    requires std::is_convertible_v<U (*)[], T (*)[]>
    constexpr explicit(false) span(span<U> other) noexcept : std::span<T>(other.data(), other.size()) {}

    // std::span will only gain this constructor in C++29 or later.
    constexpr explicit(false) span(std::initializer_list<std::remove_cv_t<T>> il) noexcept
    requires std::is_const_v<T> && (!std::same_as<std::decay_t<T>, bool>)
        : std::span<T>(il.begin(), il.end()) {}
};

template <typename It, typename EndOrSize>
span(It, EndOrSize) -> span<std::remove_reference_t<decltype(*std::declval<It&>())>>;

template <typename R>
requires std::ranges::contiguous_range<R>
span(R&&) -> span<std::remove_reference_t<decltype(*std::begin(std::declval<R&>()))>>;

/// Reinterprets the elements of a span as bytes.
template <typename T>
span<const std::byte> as_bytes(span<T> s) noexcept {  // NOLINT(readability-identifier-naming)
    return span<const std::byte>{std::as_bytes(std::span<T>{s})};
}

/// Reinterprets the elements of a span as writable bytes.
template <typename T>
requires(!std::is_const_v<T>)
span<std::byte> as_writable_bytes(span<T> s) noexcept {  // NOLINT(readability-identifier-naming)
    return span<std::byte>{std::as_writable_bytes(std::span<T>{s})};
}

}  // namespace utils

USERVER_NAMESPACE_END

// std::span must implement std::ranges::enable_borrowed_range, so <span> will pull it in.
template <typename T>
inline constexpr bool std::ranges::enable_borrowed_range<USERVER_NAMESPACE::utils::span<T>> = true;

// std::span must implement std::ranges::enable_view, so <span> will pull it in.
template <typename T>
inline constexpr bool std::ranges::enable_view<USERVER_NAMESPACE::utils::span<T>> = true;
