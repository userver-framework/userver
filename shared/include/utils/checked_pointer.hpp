#pragma once

#include <stdexcept>

#include <utils/assert.hpp>

namespace utils {

/// Utility template for returning a pointer to an object that is owned by
/// someone else.
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

  T* const Get() const& {
    CheckPointer();
    return ptr_;
  }

  T* const Get() && { RvalueDisabled(); }

  T* const operator->() const& { return Get(); }
  T* const operator->() && { RvalueDisabled(); }

  T& operator*() const& { return *Get(); }
  T& operator*() && { RvalueDisabled(); }

 private:
  [[noreturn]] void RvalueDisabled() {
    static_assert(
        false && sizeof(T),
        "Don't use temporary CheckedPtr, check it first, then dereference");
    std::abort();
  }
  void CheckPointer() const {
    UASSERT_MSG(checked_,
                "CheckedPtr contents were not checked before dereferencing");
    if (!ptr_) throw std::runtime_error{"Empty checked_pointer"};
  }
#ifndef NDEBUG
  mutable bool checked_{false};
#endif
  T* ptr_;
};

template <typename T>
class CheckedPtr<T*> {
  static_assert(false && sizeof(T), "Don't use CheckedPointer for pointers");
};

template <typename T>
class CheckedPtr<T&> {
  static_assert(false && sizeof(T), "Don't use CheckedPointer for references");
};

template <typename T>
CheckedPtr<T> MakeCheckedPtr(T* ptr) {
  return CheckedPtr<T>{ptr};
}

}  // namespace utils
