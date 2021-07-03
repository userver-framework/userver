#pragma once

/// @file utils/checked_pointer.hpp
/// @brief @copybrief utils::CheckedPtr

#include <stdexcept>

#include <utils/assert.hpp>

namespace utils {

/// @ingroup userver_containers
///
/// @brief Utility template for returning a pointer to an object that
/// is owned by someone else; throws std::runtime_error if nullptr is stored
///
/// Useful for returning cache search result.
template <typename T>
class CheckedPtr {
 public:
  /* implicit */ constexpr CheckedPtr(std::nullptr_t) noexcept
      : ptr_{nullptr} {}
  explicit constexpr CheckedPtr(T* ptr) noexcept : ptr_{ptr} {}

  explicit constexpr operator bool() const noexcept {
#ifndef NDEBUG
    checked_ = true;
#endif
    return ptr_;
  }

  T* Get() const& {
    CheckPointer();
    return ptr_;
  }

  T* Get() && { RvalueDisabled(); }

  T* operator->() const& { return Get(); }
  T* operator->() && { RvalueDisabled(); }

  T& operator*() const& { return *Get(); }
  T& operator*() && { RvalueDisabled(); }

 private:
  [[noreturn]] void RvalueDisabled() {
    static_assert(
        !sizeof(T),
        "Don't use temporary CheckedPtr, check it first, then dereference");
    std::abort();
  }
  void CheckPointer() const {
#ifndef NDEBUG
    UASSERT_MSG(checked_,
                "CheckedPtr contents were not checked before dereferencing");
#endif
    if (!ptr_) throw std::runtime_error{"Empty checked_pointer"};
  }
#ifndef NDEBUG
  mutable bool checked_{false};
#endif
  T* ptr_;
};

template <typename T>
class CheckedPtr<T*> {
  static_assert(!sizeof(T), "Don't use CheckedPointer for pointers");
};

template <typename T>
class CheckedPtr<T&> {
  static_assert(!sizeof(T), "Don't use CheckedPointer for references");
};

template <typename T>
CheckedPtr<T> MakeCheckedPtr(T* ptr) {
  return CheckedPtr<T>{ptr};
}

}  // namespace utils
