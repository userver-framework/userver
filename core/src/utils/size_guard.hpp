#pragma once

#include <atomic>
#include <memory>
#include <type_traits>

#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace detail {

template <typename T, typename = utils::void_t<>>
struct GuardedValueImpl {
  using Type = T;
};

template <typename T>
struct GuardedValueImpl<T, utils::void_t<typename T::ValueType>> {
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
  ValueType GetValue() const { return size_when_created_; }

 private:
  SizeType* size_;
  ValueType size_when_created_;
};

template <typename T>
struct SizeGuard<std::shared_ptr<std::atomic<T>>> {
  using SharedSize = std::shared_ptr<std::atomic<T>>;
  using ValueType = T;

  SizeGuard() = default;

  explicit SizeGuard(SharedSize size)
      : size_{size}, size_when_created_{size ? ++(*size) : ValueType{}} {}
  SizeGuard(SizeGuard&& rhs) noexcept
      : size_{std::move(rhs.size_)},
        size_when_created_{std::move(rhs.size_when_created_)} {}
  ~SizeGuard() {
    if (size_) {
      --(*size_);
    }
  }

  SizeGuard& operator=(const SizeGuard&) = delete;
  SizeGuard& operator=(SizeGuard&&) = delete;

  void Dismiss() { size_.reset(); }
  ValueType GetValue() const { return size_when_created_; }

 private:
  SharedSize size_;
  ValueType size_when_created_{};
};

}  // namespace utils

USERVER_NAMESPACE_END
