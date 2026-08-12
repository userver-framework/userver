#pragma once

/// @file userver/utils/required.hpp
/// @brief @copybrief utils::Required

#include <concepts>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

#include <userver/compiler/impl/lifetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief A wrapper that holds a value of type `T` and has no default
/// constructor, forcing explicit initialization.
///
/// Unlike `std::optional`, `Required` is always engaged after construction.
/// It is useful for struct fields that must be explicitly set.
///
/// ## Example usage
///
/// @snippet utils/required_test.cpp  sample
///
/// @snippet utils/required_test.cpp  sample usage
template <typename T>
class Required final {
public:
    Required() = delete;

    /// @brief Construct `T` from a value (conditionally explicit).
    template <typename U>
    requires(!std::same_as<std::decay_t<U>, Required> && std::constructible_from<T, U &&>)
    constexpr explicit(!std::convertible_to<U&&, T>) Required(U&& value)
        : value_(std::forward<U>(value))
    {}

    /// @brief Emplace-construct `T` from `args`.
    template <typename... Args>
    requires(sizeof...(Args) >= 2 && std::constructible_from<T, Args && ...>)
    constexpr explicit Required(Args&&... args)
        : value_(std::forward<Args>(args)...)
    {}

    Required(const Required&) = default;
    Required& operator=(const Required&) = default;
    Required(Required&&) = default;
    Required& operator=(Required&&) = default;

    /// @brief Access the contained value.
    constexpr T& operator*() & noexcept USERVER_IMPL_LIFETIME_BOUND { return value_; }

    /// @overload
    constexpr const T& operator*() const& noexcept USERVER_IMPL_LIFETIME_BOUND { return value_; }

    /// @overload
    constexpr T&& operator*() && noexcept { return std::move(value_); }

    /// @brief Access members of the contained value.
    constexpr T* operator->() noexcept USERVER_IMPL_LIFETIME_BOUND { return std::addressof(value_); }

    /// @overload
    constexpr const T* operator->() const noexcept USERVER_IMPL_LIFETIME_BOUND { return std::addressof(value_); }

    /// @brief Implicit conversion to `T&`.
    constexpr /*implicit*/ operator T&() & noexcept USERVER_IMPL_LIFETIME_BOUND { return value_; }

    /// @brief Implicit conversion to `const T&`.
    constexpr /*implicit*/ operator const T&() const& noexcept USERVER_IMPL_LIFETIME_BOUND { return value_; }

    /// @brief Implicit conversion to any type that `T` is implicitly convertible to (by value).
    template <typename U>
    requires(!std::same_as<U, T> && !std::is_reference_v<U> && std::convertible_to<const T&, U>)
    constexpr /*implicit*/ operator U() const {
        return value_;
    }

private:
    T value_;
};

/// @brief Convert `Required<T>` to `std::string_view` via ADL-found `ToStringView` on the inner value.
///
/// Participates in overload resolution only when `ToStringView(*req)` is valid.
template <typename T>
requires requires(const T& v) {
    {
        ToStringView(v)
    } -> std::same_as<std::string_view>;
}
std::string_view ToStringView(const Required<T>& req) {
    return ToStringView(*req);
}

}  // namespace utils

USERVER_NAMESPACE_END
