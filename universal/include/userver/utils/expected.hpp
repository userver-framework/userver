#pragma once

/// @file userver/utils/expected.hpp
/// @brief @copybrief utils::expected

#include <concepts>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

// NOLINTBEGIN(readability-identifier-naming)

namespace impl {

// Standard library implementations of std::cmp_equal are not required to be SFINAE-friendly (e.g. libstdc++
// enforces the "both types are standard integers" precondition via a body-level static_assert instead of a
// constrained declaration), so `requires { std::cmp_equal(lhs, rhs); }` is not a portable way to detect
// applicability. Do the check manually instead, following the exact rules of [utility.intcmp].
template <class T>
concept StandardInteger =
    std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char> && !std::same_as<T, wchar_t> &&
    !std::same_as<T, char8_t> && !std::same_as<T, char16_t> && !std::same_as<T, char32_t>;

/// @brief Same as `lhs == rhs`, but avoids `-Wsign-compare` warnings when comparing integers of different
/// signedness by using `std::cmp_equal` where applicable.
template <class T, class U>
constexpr bool ExpectedEqual(const T& lhs, const U& rhs) {
    if constexpr (StandardInteger<T> && StandardInteger<U>) {
        return std::cmp_equal(lhs, rhs);
    } else {
        return lhs == rhs;
    }
}

}  // namespace impl

class bad_expected_access : public std::exception {
public:
    using std::exception::exception;

    explicit bad_expected_access(const std::string& message)
        : message_{message}
    {}

    const char* what() const noexcept override;

private:
    std::string message_;
};

template <class E>
class [[nodiscard]] unexpected {
public:
    unexpected(const E& error);
    unexpected(E&& error);

    template <class... Args>
    unexpected(Args&&... args);

    template <class U, class... Args>
    unexpected(std::initializer_list<U> il, Args&&... args);

    E& error() noexcept USERVER_IMPL_LIFETIME_BOUND;
    const E& error() const noexcept USERVER_IMPL_LIFETIME_BOUND;

private:
    E value_;
};

template <class E>
unexpected(E) -> unexpected<E>;

/// @ingroup userver_universal userver_containers
///
/// @brief For holding a value or an error
template <class S, class E>
class [[nodiscard]] expected {
public:
    constexpr expected();
    expected(const S& success);
    expected(S&& success);
    expected(const unexpected<E>& error);
    expected(unexpected<E>&& error);

    template <class G>
    requires std::is_convertible_v<G, E>
    expected(const unexpected<G>& error);

    template <class G>
    requires std::is_convertible_v<G, E>
    expected(unexpected<G>&& error);

    /// @brief Check whether *this contains an expected value
    bool has_value() const noexcept;

    /// @brief Check whether *this contains an expected value
    explicit operator bool() const noexcept;

    /// @brief Return reference to the value or throws bad_expected_access
    /// if it's not available
    /// @throws utils::bad_expected_access if *this contain an unexpected value
    S& value() & USERVER_IMPL_LIFETIME_BOUND;

    /// @overload
    S&& value() && USERVER_IMPL_LIFETIME_BOUND;

    /// @overload
    const S& value() const& USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Unchecked access to the contained value.
    /// @warning The behavior is undefined if `*this` does not contain a value; use @ref expected::value() for checked
    /// access.
    S& operator*() & noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @overload
    S&& operator*() && noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @overload
    const S& operator*() const& noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Unchecked access to the members of the contained value.
    /// @warning The behavior is undefined if `*this` does not contain a value; use @ref expected::value() for checked
    /// access.
    S* operator->() noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @overload
    const S* operator->() const noexcept USERVER_IMPL_LIFETIME_BOUND;

    /// @brief Return reference to the error value or throws bad_expected_access
    /// if it's not available
    /// @throws utils::bad_expected_access if success value is not available
    E& error() USERVER_IMPL_LIFETIME_BOUND;

    /// @overload
    const E& error() const USERVER_IMPL_LIFETIME_BOUND;

private:
    std::variant<S, unexpected<E>> data_;
};

template <class E>
class [[nodiscard]] expected<void, E> {
public:
    constexpr expected() noexcept;
    expected(const unexpected<E>& error);
    expected(unexpected<E>&& error);

    template <class G>
    requires std::is_convertible_v<G, E>
    expected(const unexpected<G>& error);

    template <class G>
    requires std::is_convertible_v<G, E>
    expected(unexpected<G>&& error);

    bool has_value() const noexcept;
    explicit operator bool() const noexcept;
    void value() const;

    E& error() USERVER_IMPL_LIFETIME_BOUND;
    const E& error() const USERVER_IMPL_LIFETIME_BOUND;

private:
    std::variant<std::monostate, unexpected<E>> data_;
};

template <class E>
unexpected<E>::unexpected(const E& error)
    : value_{error}
{}

template <class E>
unexpected<E>::unexpected(E&& error)
    : value_{std::forward<E>(error)}
{}

template <class E>
template <class... Args>
unexpected<E>::unexpected(Args&&... args)
    : value_(std::forward<Args>(args)...)
{}

template <class E>
template <class U, class... Args>
unexpected<E>::unexpected(std::initializer_list<U> il, Args&&... args)
    : value_(il, std::forward<Args>(args)...)
{}

template <class E>
E& unexpected<E>::error() noexcept USERVER_IMPL_LIFETIME_BOUND {
    return value_;
}

template <class E>
const E& unexpected<E>::error() const noexcept USERVER_IMPL_LIFETIME_BOUND {
    return value_;
}

template <class S, class E>
constexpr expected<S, E>::expected()
    : data_(std::in_place_index<0>)
{}

template <class S, class E>
expected<S, E>::expected(const S& success)
    : data_(success)
{}

template <class S, class E>
expected<S, E>::expected(S&& success)
    : data_(std::forward<S>(success))
{}

template <class S, class E>
expected<S, E>::expected(const unexpected<E>& error)
    : data_(error.error())
{}

template <class S, class E>
expected<S, E>::expected(unexpected<E>&& error)
    : data_(std::forward<unexpected<E>>(error.error()))
{}

template <class S, class E>
template <class G>
requires std::is_convertible_v<G, E>
expected<S, E>::expected(const unexpected<G>& error)
    : data_(utils::unexpected<E>(std::forward<G>(error.error())))
{}

template <class S, class E>
template <class G>
requires std::is_convertible_v<G, E>
expected<S, E>::expected(unexpected<G>&& error)
    : data_(utils::unexpected<E>(std::forward<G>(error.error())))
{}

template <class S, class E>
bool expected<S, E>::has_value() const noexcept {
    return std::holds_alternative<S>(data_);
}

template <class S, class E>
expected<S, E>::operator bool() const noexcept {
    return has_value();
}

template <class S, class E>
    S& expected<S, E>::value() & USERVER_IMPL_LIFETIME_BOUND {
    S* result = std::get_if<S>(&data_);
    if (result == nullptr) {
        throw bad_expected_access("Trying to get undefined value from utils::expected");
    }
    return *result;
}

template <class S, class E>
    S&& expected<S, E>::value() && USERVER_IMPL_LIFETIME_BOUND {
    return std::move(value());
}

template <class S, class E>
const S& expected<S, E>::value() const& USERVER_IMPL_LIFETIME_BOUND {
    const S* result = std::get_if<S>(&data_);
    if (result == nullptr) {
        throw bad_expected_access("Trying to get undefined value from utils::expected");
    }
    return *result;
}

template <class S, class E>
S& expected<S, E>::operator*() & noexcept USERVER_IMPL_LIFETIME_BOUND {
    UASSERT_MSG(has_value(), "Trying to dereference utils::expected that does not contain a value");
    return *std::get_if<S>(&data_);
}

template <class S, class E>
S&& expected<S, E>::operator*() && noexcept USERVER_IMPL_LIFETIME_BOUND {
    return std::move(**this);
}

template <class S, class E>
const S& expected<S, E>::operator*() const& noexcept USERVER_IMPL_LIFETIME_BOUND {
    UASSERT_MSG(has_value(), "Trying to dereference utils::expected that does not contain a value");
    return *std::get_if<S>(&data_);
}

template <class S, class E>
S* expected<S, E>::operator->() noexcept USERVER_IMPL_LIFETIME_BOUND {
    UASSERT_MSG(has_value(), "Trying to dereference utils::expected that does not contain a value");
    return std::get_if<S>(&data_);
}

template <class S, class E>
const S* expected<S, E>::operator->() const noexcept USERVER_IMPL_LIFETIME_BOUND {
    UASSERT_MSG(has_value(), "Trying to dereference utils::expected that does not contain a value");
    return std::get_if<S>(&data_);
}

template <class S, class E>
E& expected<S, E>::error() USERVER_IMPL_LIFETIME_BOUND {
    auto* result = std::get_if<unexpected<E>>(&data_);
    if (result == nullptr) {
        throw bad_expected_access("Trying to get undefined error value from utils::expected");
    }
    return result->error();
}

template <class S, class E>
const E& expected<S, E>::error() const USERVER_IMPL_LIFETIME_BOUND {
    const auto* result = std::get_if<unexpected<E>>(&data_);
    if (result == nullptr) {
        throw bad_expected_access("Trying to get undefined error value from utils::expected");
    }
    return result->error();
}

/// @brief Compares two utils::expected: equal either if both hold equal values, or if both hold equal errors.
///
/// @note `operator!=` and the reversed-argument-order overload are synthesized by the compiler (C++20 rewritten
/// comparison operators), no need to define them separately.
template <class S, class E>
requires std::equality_comparable<S> && std::equality_comparable<E>
bool operator==(const expected<S, E>& lhs, const expected<S, E>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    return lhs.has_value() ? impl::ExpectedEqual(*lhs, *rhs) : impl::ExpectedEqual(lhs.error(), rhs.error());
}

/// @brief Compares utils::expected with a value: equal if `lhs` holds a value equal to `rhs`.
///
/// @note `operator!=` and the reversed-argument-order overload (`rhs == lhs`) are synthesized by the compiler
/// (C++20 rewritten comparison operators), no need to define them separately.
template <class S, class E, class T>
requires(!std::same_as<T, expected<S, E>>) && std::equality_comparable_with<S, T>
bool operator==(const expected<S, E>& lhs, const T& rhs) {
    return lhs.has_value() && impl::ExpectedEqual(*lhs, rhs);
}

/// @brief Compares utils::expected with utils::unexpected: equal if `lhs` holds an error equal to `rhs.error()`.
///
/// @note `operator!=` and the reversed-argument-order overload (`rhs == lhs`) are synthesized by the compiler
/// (C++20 rewritten comparison operators), no need to define them separately.
template <class S, class E, class G>
requires std::equality_comparable_with<E, G>
bool operator==(const expected<S, E>& lhs, const unexpected<G>& rhs) {
    return !lhs.has_value() && impl::ExpectedEqual(lhs.error(), rhs.error());
}

template <class E>
constexpr expected<void, E>::expected() noexcept: data_(std::in_place_index<0>) {}

template <class E>
expected<void, E>::expected(const unexpected<E>& error)
    : data_(error.error())
{}

template <class E>
expected<void, E>::expected(unexpected<E>&& error)
    : data_(std::forward<unexpected<E>>(error.error()))
{}

template <class E>
template <class G>
requires std::is_convertible_v<G, E>
expected<void, E>::expected(const unexpected<G>& error)
    : data_(utils::unexpected<E>(std::forward<G>(error.error())))
{}

template <class E>
template <class G>
requires std::is_convertible_v<G, E>
expected<void, E>::expected(unexpected<G>&& error)
    : data_(utils::unexpected<E>(std::forward<G>(error.error())))
{}

template <class E>
bool expected<void, E>::has_value() const noexcept {
    return data_.index() == 0;
}

template <class E>
expected<void, E>::operator bool() const noexcept {
    return has_value();
}

template <class E>
void expected<void, E>::value() const {
    if (!has_value()) {
        throw bad_expected_access("Trying to get undefined value from utils::expected");
    }
}

template <class E>
E& expected<void, E>::error() USERVER_IMPL_LIFETIME_BOUND {
    auto* result = std::get_if<unexpected<E>>(&data_);
    if (result == nullptr) {
        throw bad_expected_access("Trying to get undefined error value from utils::expected");
    }
    return result->error();
}

template <class E>
const E& expected<void, E>::error() const USERVER_IMPL_LIFETIME_BOUND {
    const auto* result = std::get_if<unexpected<E>>(&data_);
    if (result == nullptr) {
        throw bad_expected_access("Trying to get undefined error value from utils::expected");
    }
    return result->error();
}

/// @brief Compares two utils::expected<void, E>: equal either if both hold a value, or if both hold equal errors.
///
/// @note `operator!=` is synthesized by the compiler (C++20 rewritten comparison operators), no need to define it
/// separately.
template <class E>
requires std::equality_comparable<E>
bool operator==(const expected<void, E>& lhs, const expected<void, E>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    return lhs.has_value() || impl::ExpectedEqual(lhs.error(), rhs.error());
}

/// @brief Compares utils::expected<void, E> with utils::unexpected: equal if `lhs` holds an error equal to
/// `rhs.error()`.
///
/// @note `operator!=` and the reversed-argument-order overload (`rhs == lhs`) are synthesized by the compiler
/// (C++20 rewritten comparison operators), no need to define them separately.
template <class E, class G>
requires std::equality_comparable_with<E, G>
bool operator==(const expected<void, E>& lhs, const unexpected<G>& rhs) {
    return !lhs.has_value() && impl::ExpectedEqual(lhs.error(), rhs.error());
}

// NOLINTEND(readability-identifier-naming)

}  // namespace utils

USERVER_NAMESPACE_END
