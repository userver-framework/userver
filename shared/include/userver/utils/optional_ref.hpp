#pragma once

/// @file userver/utils/optional_ref.hpp
/// @brief @copybrief utils::OptionalRef

#include <optional>
#include <type_traits>

#include <boost/optional/optional_fwd.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
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
  static_assert(!std::is_reference<T>::value, "Do not use a reference for T");

  constexpr OptionalRef() noexcept = default;
  constexpr OptionalRef(std::nullopt_t) noexcept {}
  constexpr OptionalRef(const OptionalRef&) noexcept = default;
  constexpr OptionalRef& operator=(const OptionalRef&) noexcept = delete;

  constexpr OptionalRef(T& other) noexcept : data_(&other) {}

  // Forming a reference to a temporary is forbidden
  explicit constexpr OptionalRef(const T&&) = delete;

  template <typename U>
  explicit constexpr OptionalRef(const std::optional<U>& other) noexcept
      : data_(GetPointer(other)) {}

  template <typename U>
  explicit constexpr OptionalRef(std::optional<U>& other) noexcept
      : data_(GetPointer(other)) {}

  template <typename U>
  explicit constexpr OptionalRef(const std::optional<U>&&) noexcept {
    static_assert(!sizeof(U), "Forming a reference to a temporary");
  }

  template <typename U>
  explicit constexpr OptionalRef(const boost::optional<U>& other) noexcept
      : data_(GetPointer(other)) {}

  template <typename U>
  explicit constexpr OptionalRef(boost::optional<U>& other) noexcept
      : data_(GetPointer(other)) {}

  template <typename U>
  explicit constexpr OptionalRef(const boost::optional<U>&&) noexcept {
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

 private:
  template <class Optional>
  static T* GetPointer(Optional& other) noexcept {
    using ValueType = decltype(*other);
    static_assert(
        std::is_const<T>::value || !std::is_const<ValueType>::value,
        "Attempt to initialize non-const T from a const optional value");

    if (!other) {
      return nullptr;
    }

    auto& value = *other;
    return &value;
  }

  T* const data_ = nullptr;
};

template <class T, class U>
constexpr bool operator==(OptionalRef<T> lhs, OptionalRef<U> rhs) noexcept {
  if (!lhs || !rhs) return !lhs && !rhs;
  return *lhs == *rhs;
}

template <class T, class U>
constexpr bool operator!=(OptionalRef<T> lhs, OptionalRef<U> rhs) noexcept {
  return !(lhs == rhs);
}

}  // namespace utils

USERVER_NAMESPACE_END
