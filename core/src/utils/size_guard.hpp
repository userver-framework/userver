#pragma once

#include <atomic>
#include <type_traits>

#include <utils/void_t.hpp>

namespace utils {

namespace detail {

template <typename T, typename = ::utils::void_t<>>
struct GuardedValueImpl {
  using Type = T;
};

template <typename T>
struct GuardedValueImpl<T, ::utils::void_t<typename T::ValueType>> {
  using Type = typename T::ValueType;
};

template <typename T>
struct GuardedValue : GuardedValueImpl<T> {};

template <typename T>
struct GuardedValue<std::atomic<T>> {
  using Type = T;
};

template <typename T>
using GuardedValueType = typename GuardedValue<T>::Type;

}  // namespace detail

template <typename T>
struct SizeGuard {
  using SizeType = T;
  using ValueType = detail::GuardedValueType<T>;
  struct DontIncrement {};

  explicit SizeGuard(SizeType& size)
      : size_{&size}, size_when_created_{++size} {}

  // A very ugly signature for a decrement-only guard
  SizeGuard(SizeType& size, DontIncrement)
      : size_{&size}, size_when_created_{size} {}

  SizeGuard(SizeGuard&& rhs) noexcept(
      std::is_nothrow_move_constructible<ValueType>::value)
      : size_{rhs.size_},
        size_when_created_{std::move(rhs.size_when_created_)} {
    rhs.Dismiss();
  }

  ~SizeGuard() {
    if (size_) {
      --(*size_);
    }
  }

  SizeGuard& operator=(const SizeGuard&) = delete;
  SizeGuard& operator=(SizeGuard&&) = delete;

  void Dismiss() { size_ = nullptr; }
  ValueType GetSize() const { return size_when_created_; }

 private:
  SizeType* size_;
  ValueType size_when_created_;
};

}  // namespace utils
