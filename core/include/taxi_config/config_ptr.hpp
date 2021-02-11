#pragma once

#include <memory>
#include <type_traits>

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
class ReadablePtr {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>, "ReadablePtr already adds `const`");

 public:
  ReadablePtr(ReadablePtr&&) noexcept = default;
  ReadablePtr& operator=(ReadablePtr&&) noexcept = default;

  const T& operator*() const& { return *config_; }
  const T& operator*() && { ReportMisuse(); }

  const T* operator->() const& { return config_; }
  const T* operator->() && { ReportMisuse(); }

 private:
  explicit ReadablePtr(impl::Storage& storage)
      : container_(storage.Read()),
        config_(&(**container_).template Get<T>()) {}

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
class Variable {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>, "Variable already adds `const`");

 public:
  explicit Variable(const components::ComponentContext& context)
      : storage_(&impl::FindStorage(context)) {}

  // trivially copyable
  Variable(const Variable&) = default;
  Variable& operator=(const Variable&) = default;

  ReadablePtr<T> Get() const { return ReadablePtr<T>{*storage_}; }

 private:
  explicit Variable(const impl::Storage& storage) : storage_(&storage) {}

  // for the constructor
  friend class Storage;

  const impl::Storage* storage_;
};

}  // namespace taxi_config
