#pragma once

/// @file userver/utils/optional_ref.hpp
/// @brief @copybrief utils::OptionalRef

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <boost/optional/optional_fwd.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal userver_containers
///
/// @brief Class that behaves as a nullable reference. Main difference from the
/// pointer - value comparison of pointed values.
///
/// Initializes from reference to a T or from optional<T>.
///
/// Once the reference is constructed it can not be changed to point to
/// different address.
template <class T>
class OptionalRef {
public:
    using value_type = T;

    static_assert(!std::is_reference<T>::value, "Do not use a reference for T");

    constexpr OptionalRef() noexcept = default;
    constexpr OptionalRef(std::nullopt_t) noexcept {}
    constexpr OptionalRef(const OptionalRef&) noexcept = default;
    constexpr OptionalRef& operator=(const OptionalRef&) noexcept = delete;

    constexpr OptionalRef(T& other) noexcept : data_(std::addressof(other)) {}

    // Forming a reference to a temporary is forbidden
    constexpr explicit OptionalRef(const T&&) = delete;

    template <typename U>
    constexpr explicit OptionalRef(const std::optional<U>& other) noexcept : data_(GetPointer(other)) {}

    template <typename U>
    constexpr explicit OptionalRef(std::optional<U>& other) noexcept : data_(GetPointer(other)) {}

    template <typename U>
    constexpr explicit OptionalRef(const std::optional<U>&&) noexcept {
        static_assert(!sizeof(U), "Forming a reference to a temporary");
    }

    template <typename U>
    constexpr explicit OptionalRef(const boost::optional<U>& other) noexcept : data_(GetPointer(other)) {}

    template <typename U>
    constexpr explicit OptionalRef(boost::optional<U>& other) noexcept : data_(GetPointer(other)) {}

    template <typename U>
    constexpr explicit OptionalRef(const boost::optional<U>&&) noexcept {
        static_assert(!sizeof(U), "Forming a reference to a temporary");
    }

    constexpr bool has_value() const noexcept { return !!data_; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr T* operator->() const {
        UASSERT(data_);
        return data_;
    }

    constexpr T& operator*() const {
        UASSERT(data_);
        return *data_;
    }

    constexpr T& value() const {
        if (!has_value()) {
            throw std::bad_optional_access();
        }

        return *data_;
    }

    template <typename U>
    constexpr T value_or(U&& default_value) const {
        if (!has_value()) {
            return std::forward<U>(default_value);
        }

        return *data_;
    }

    template <typename F>
    constexpr auto and_then(F&& function) const -> std::remove_cvref_t<std::invoke_result_t<F, T&>> {
        using Result = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        static_assert(
            meta::IsInstantiationOf<Result, std::optional> || meta::IsInstantiationOf<Result, OptionalRef>,
            "The function passed to and_then must return std::optional or utils::OptionalRef"
        );

        if (has_value()) {
            return std::invoke(std::forward<F>(function), *data_);
        }
        return Result{};
    }

    template <typename F>
    constexpr auto transform(F&& function) const {
        using Result = std::remove_cv_t<std::invoke_result_t<F, T&>>;

        if (has_value()) {
            return std::optional<Result>{std::invoke(std::forward<F>(function), *data_)};
        }
        return std::optional<Result>{};
    }

    template <typename F>
    constexpr OptionalRef or_else(F&& function) const {
        using Result = std::remove_cvref_t<std::invoke_result_t<F>>;
        static_assert(
            std::is_same_v<Result, OptionalRef>,
            "The function passed to or_else must return utils::OptionalRef<T>"
        );

        if (has_value()) {
            return *this;
        }
        return std::invoke(std::forward<F>(function));
    }

private:
    template <class Optional>
    static T* GetPointer(Optional& other) noexcept {
        using ValueType = decltype(*other);
        static_assert(
            std::is_const<T>::value || !std::is_const<ValueType>::value,
            "Attempt to initialize non-const T from a const optional value"
        );

        return other.has_value() ? std::addressof(*other) : nullptr;
    }

    T* const data_ = nullptr;
};

template <class T, class U>
constexpr bool operator==(OptionalRef<T> lhs, OptionalRef<U> rhs) noexcept {
    if (!lhs || !rhs) {
        return !lhs && !rhs;
    }
    return *lhs == *rhs;
}

}  // namespace utils

USERVER_NAMESPACE_END
