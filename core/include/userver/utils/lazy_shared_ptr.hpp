#pragma once

/// @file userver/utils/lazy_shared_ptr.hpp
/// @brief @copybrief utils::LazySharedPtr

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include <userver/engine/sleep.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/shared_readable_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_containers
///
/// @brief A lazy wrapper around utils::SharedReadablePtr that fetches the data
/// on first access.
///
/// Provides standard thread-safety guarantee: `const` methods can be called
/// concurrently. Once fetched, the same data is guaranteed to be returned
/// unless the pointer is assigned to.
template <typename T>
class LazySharedPtr final {
 public:
  /// @brief The default constructor, initializes with `nullptr`.
  LazySharedPtr() noexcept
      : value_(nullptr), shared_filled_(true), shared_(), get_data_() {}

  /// @brief The non-lazy constructor
  LazySharedPtr(utils::SharedReadablePtr<T> ptr) noexcept
      : value_(ptr.Get()), shared_filled_(true), shared_(std::move(ptr)) {}

  /// @brief The lazy constructor
  LazySharedPtr(std::function<utils::SharedReadablePtr<T>()> function) noexcept
      : get_data_(std::move(function)) {}

  /// @brief The copy constructor.
  /// @note If `other` has not been fetched yet, `this` will not fetch
  /// immediately, so `this` and `other` may end up pointing to different
  /// objects.
  LazySharedPtr(const LazySharedPtr& other) : get_data_(other.get_data_) {
    if (other.shared_filled_.load()) {
      value_.store(other.shared_.Get());
      shared_ = other.shared_;
      shared_filled_.store(true);
    }
  }

  /// @brief The move constructor.
  LazySharedPtr(LazySharedPtr&& other) noexcept
      : value_(other.value_.load()),
        shared_filled_(other.shared_filled_.load()),
        shared_(std::move(other.shared_)),
        get_data_(std::move(other.get_data_)) {}

  /// @brief The copy assignment operator.
  /// @note Like copy-constructor, it does not generate pointers if `other` do
  /// not generate them.
  LazySharedPtr& operator=(const LazySharedPtr& other) {
    *this = LazySharedPtr(other);
    return *this;
  }

  /// @brief The move assignment operator.
  LazySharedPtr& operator=(LazySharedPtr&& other) noexcept {
    value_.store(other.value_.load(), std::memory_order_relaxed);
    shared_filled_.store(other.shared_filled_.load(),
                         std::memory_order_relaxed);
    shared_ = std::move(other.shared_);
    get_data_ = std::move(other.get_data_);
  }

  /// @brief Get a pointer to the data (may be null). Fetches the data on the
  /// first access.
  const T* Get() const& {
    if (const auto* current_value = value_.load(); current_value != kUnfilled) {
      return current_value;
    }
    auto readable = get_data_();
    if (const auto* expected = kUnfilled;
        value_.compare_exchange_strong(expected, readable.Get())) {
      shared_ = std::move(readable);
      shared_filled_.store(true);
    }
    return value_;
  }

  /// @brief Get a smart pointer to the data (may be null). Fetches the data on
  /// the first access.
  const utils::SharedReadablePtr<T>& GetShared() const& {
    if (value_ == kUnfilled) {
      auto readable = get_data_();
      if (const auto* expected = kUnfilled;
          value_.compare_exchange_strong(expected, readable.Get())) {
        shared_ = std::move(readable);
        shared_filled_.store(true);
      }
    }
    while (!shared_filled_.load()) {
      // Another thread has filled 'value_', but not yet 'shared_'. It will be
      // filled in a few CPU cycles. Discovering LazySharedPtr in such a state
      // is expected to be rare.
      engine::Yield();
    }
    return shared_;
  }

  ///@returns `*Get()`
  ///@note `Get()` must not be `nullptr`.
  const T& operator*() const& {
    const auto* res = Get();
    UASSERT(res);
    return *res;
  }

  /// @returns `Get()`
  /// @note `Get()` must not be `nullptr`.
  const T* operator->() const& { return &**this; }

  /// @returns `Get() != nullptr`
  explicit operator bool() const& { return !!Get(); }

  /// @returns `GetShared()`
  operator const utils::SharedReadablePtr<T>&() const& { return GetShared(); }

  /// @returns `GetShared()`
  operator std::shared_ptr<const T>() const& { return GetShared(); }

 private:
  static inline const auto kUnfilled =
      reinterpret_cast<const T*>(std::uintptr_t{1});

  mutable std::atomic<const T*> value_{kUnfilled};
  mutable std::atomic<bool> shared_filled_{false};
  mutable utils::SharedReadablePtr<T> shared_{nullptr};
  std::function<utils::SharedReadablePtr<T>()> get_data_{};
};

/// @brief Make a lazy pointer to the data of a cache.
///
/// The cache type must have:
/// - `DataType` member type;
/// - `Get() -> utils::SharedReadablePtr<DataType>` method.
///
/// For example, components::CachingComponentBase satisfies these requirements.
template <typename Cache>
LazySharedPtr<typename Cache::DataType> MakeLazyCachePtr(Cache& cache) {
  return utils::LazySharedPtr<typename Cache::DataType>(
      [&cache] { return cache.Get(); });
}

}  // namespace utils

USERVER_NAMESPACE_END
