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
class Source;

namespace impl {

using Storage = rcu::Variable<Config>;

const Storage& FindStorage(const components::ComponentContext& context);

}  // namespace impl

/// Owns a snapshot of config. You may use operator*() or operator->()
/// to do something with the config.
template <typename T>
class SnapshotPtr final {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>, "SnapshotPtr already adds `const`");

 public:
  /// Convert from a ReadablePtr of parent config
  template <typename U,
            typename = std::enable_if_t<std::is_same_v<U, taxi_config::Config>>>
  explicit SnapshotPtr(SnapshotPtr<U>&& parent)
      : container_(std::move(parent.container_)), config_(&Get(*container_)) {}

  SnapshotPtr(SnapshotPtr&&) noexcept = default;
  SnapshotPtr& operator=(SnapshotPtr&&) noexcept = default;

  const T& operator*() const& { return *config_; }
  const T& operator*() && { ReportMisuse(); }

  const T* operator->() const& { return config_; }
  const T* operator->() && { ReportMisuse(); }

  T Copy() const { return *config_; }

 private:
  explicit SnapshotPtr(const impl::Storage& storage)
      : container_(storage.Read()), config_(&Get(*container_)) {}

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
  friend class Source<T>;

  rcu::ReadablePtr<Config> container_;
  const T* config_;
};

/// A helper for easy config fetching in components. After construction, Source
/// can be copied around and passed to clients or child helper classes.
template <typename T>
class Source final {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>, "Source already adds `const`");

 public:
  explicit Source(const components::ComponentContext& context)
      : storage_(&impl::FindStorage(context)) {}

  /// Convert from a Source of parent config
  template <typename U,
            typename = std::enable_if_t<std::is_same_v<U, taxi_config::Config>>>
  explicit Source(const Source<U>& parent) : storage_(parent.storage_) {}

  // trivially copyable
  Source(const Source&) = default;
  Source& operator=(const Source&) = default;

  SnapshotPtr<T> GetSnapshot() const { return SnapshotPtr<T>{*storage_}; }

  T GetCopy() const { return GetSnapshot().Copy(); }

 private:
  explicit Source(const impl::Storage& storage) : storage_(&storage) {}

  // for the constructor
  friend class StorageMock;

  const impl::Storage* storage_;
};

}  // namespace taxi_config
