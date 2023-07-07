#pragma once

/// @file userver/utils/not_null.hpp
/// @brief @copybrief utils::NotNull

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief Restricts a pointer or smart pointer to only hold non-null values.
template <typename T>
class NotNull {
  static_assert(!std::is_reference_v<T>,
                "NotNull does not work with references");
  static_assert(!std::is_const_v<T>);

 public:
  constexpr explicit NotNull() = delete;

  constexpr explicit NotNull(const T& u) : ptr_(u) {
    UASSERT_MSG(ptr_, "Trying to construct NotNull from null");
  }

  constexpr explicit NotNull(T&& u) : ptr_(std::move(u)) {
    UASSERT_MSG(ptr_, "Trying to construct NotNull from null");
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U, T>>>
  constexpr explicit NotNull(U&& u) : ptr_(std::forward<U>(u)) {
    UASSERT_MSG(ptr_, "Trying to construct NotNull from null");
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T>>>
  constexpr /*implicit*/ NotNull(U& u) : ptr_(std::addressof(u)) {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U, T>>>
  constexpr NotNull(const NotNull<U>& other) : ptr_(other.GetBase()) {
    UASSERT_MSG(ptr_,
                "Trying to construct NotNull from null (moved-from) NotNull");
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U, T>>>
  constexpr NotNull(NotNull<U>&& other) : ptr_(std::move(other).GetBase()) {
    UASSERT_MSG(ptr_,
                "Trying to construct NotNull from null (moved-from) NotNull");
  }

  constexpr NotNull(std::nullptr_t) = delete;

  NotNull(const NotNull& other) noexcept = default;
  NotNull(NotNull&& other) noexcept = default;

  NotNull& operator=(const NotNull& other) noexcept = default;
  NotNull& operator=(NotNull&& other) noexcept = default;

  constexpr NotNull& operator=(std::nullptr_t) = delete;

  constexpr const T& GetBase() const& {
    UASSERT_MSG(ptr_, "Trying to access a null (moved-from) NotNull");
    return ptr_;
  }

  constexpr T&& GetBase() && {
    UASSERT_MSG(ptr_, "Trying to access a null (moved-from) NotNull");
    return std::move(ptr_);
  }

  constexpr /*implicit*/ operator const T&() const& { return GetBase(); }

  constexpr /*implicit*/ operator bool() = delete;

  constexpr decltype(auto) operator->() const& { return GetBase(); }

  constexpr decltype(auto) operator*() const& { return *GetBase(); }

  template <typename U>
  constexpr bool operator==(const NotNull<U>& other) const& {
    return GetBase() == other.GetBase();
  }

  template <typename U>
  constexpr bool operator!=(const NotNull<U>& other) const& {
    return GetBase() != other.GetBase();
  }

 private:
  T ptr_;
};

/// @ingroup userver_containers
///
/// @brief A `std::shared_ptr` that is guaranteed to be not-null.
/// @see MakeSharedRef
template <typename U>
using SharedRef = NotNull<std::shared_ptr<U>>;

/// @ingroup userver_containers
///
/// @brief A `std::unique_ptr` that is guaranteed to be not-null.
/// @see MakeUniqueRef
template <typename U>
using UniqueRef = NotNull<std::unique_ptr<U>>;

/// @brief An equivalent of `std::make_shared` for SharedRef.
template <typename U, typename... Args>
SharedRef<U> MakeSharedRef(Args&&... args) {
  return SharedRef<U>{std::make_shared<U>(std::forward<Args>(args)...)};
}

/// @brief An equivalent of `std::make_unique` for UniqueRef.
template <typename U, typename... Args>
UniqueRef<U> MakeUniqueRef(Args&&... args) {
  return UniqueRef<U>{std::make_unique<U>(std::forward<Args>(args)...)};
}

}  // namespace utils

USERVER_NAMESPACE_END

template <typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<USERVER_NAMESPACE::utils::NotNull<T>> : public std::hash<T> {
  using std::hash<T>::hash;

  auto operator()(const USERVER_NAMESPACE::utils::NotNull<T>& value) const
      noexcept(std::is_nothrow_invocable_v<const std::hash<T>&, const T&>) {
    return this->std::hash<T>::operator()(value.GetBase());
  }
};
