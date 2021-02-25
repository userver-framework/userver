#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include <rcu/rcu.hpp>
#include <taxi_config/config.hpp>

namespace components {
class ComponentContext;
class TaxiConfig;
}  // namespace components

namespace taxi_config {

template <typename T>
class Variable;

namespace impl {

using Storage = rcu::Variable<std::shared_ptr<const Config>>;

const Storage& FindStorage(const components::ComponentContext& context);

}  // namespace impl

/// Handle to a snapshot of config. You may use operator*() or operator->()
/// to do something with the stored config.
template <typename T>
class ReadablePtr final {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>, "ReadablePtr already adds `const`");

 public:
  /// Convert from a ReadablePtr of parent config
  template <typename U,
            typename = std::enable_if_t<std::is_same_v<U, taxi_config::Config>>>
  explicit ReadablePtr(ReadablePtr<U>&& parent)
      : container_(std::move(parent.container_)), config_(&Get(**container_)) {}

  ReadablePtr(ReadablePtr&&) noexcept = default;
  ReadablePtr& operator=(ReadablePtr&&) noexcept = default;

  const T& operator*() const& { return *config_; }
  const T& operator*() && { ReportMisuse(); }

  const T* operator->() const& { return config_; }
  const T* operator->() && { ReportMisuse(); }

  T Copy() const { return *config_; }

 private:
  explicit ReadablePtr(const impl::Storage& storage)
      : container_(storage.Read()), config_(&Get(**container_)) {}

  static const T& Get(const Config& config) {
    if constexpr (std::is_same_v<T, Config>) {
      return config;
    } else {
      return config.template Get<T>();
    }
  }

  [[noreturn]] static void ReportMisuse() {
    static_assert(!sizeof(T), "keep the pointer before using, please");
  }

  // for the constructor
  friend class Variable<T>;

  rcu::ReadablePtr<std::shared_ptr<const Config>> container_;
  const T* config_;
};

/// A helper for easy config fetching in components. After construction,
/// the Variable can be copied around and passed to clients or child helper
/// classes.
template <typename T>
class Variable final {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>, "Variable already adds `const`");

 public:
  explicit Variable(const components::ComponentContext& context)
      : storage_(&impl::FindStorage(context)) {}

  /// Convert from a Variable of parent config
  template <typename U,
            typename = std::enable_if_t<std::is_same_v<U, taxi_config::Config>>>
  explicit Variable(const Variable<U>& parent) : storage_(parent.storage_) {}

  // trivially copyable
  Variable(const Variable&) = default;
  Variable& operator=(const Variable&) = default;

  ReadablePtr<T> Get() const { return ReadablePtr<T>{*storage_}; }

  T GetCopy() const { return ReadablePtr<T>{*storage_}.Copy(); }

 private:
  explicit Variable(const impl::Storage& storage) : storage_(&storage) {}

  // for the constructor
  friend class StorageMock;

  const impl::Storage* storage_;
};

}  // namespace taxi_config
