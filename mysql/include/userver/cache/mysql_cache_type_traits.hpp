#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <memory_resource>
#include <unordered_map>

#include <userver/formats/parse/to.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/void_t.hpp>

#include <userver/cache/caching_component_base.hpp>

#include <userver/storages/mysql/cluster_host_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace mysql_cache::impl {

template <typename T>
using ValueType = typename T::ValueType;
template <typename T>
inline constexpr bool kHasValueType = meta::kIsDetected<ValueType, T>;

template <typename T>
using RawValueTypeImpl = typename T::RawValueType;
template <typename T>
inline constexpr bool kHasRawValueType = meta::kIsDetected<RawValueTypeImpl, T>;
template <typename T>
using RawValueType = meta::DetectedOr<ValueType<T>, RawValueTypeImpl, T>;

template <typename MySQLCachePolicy>
auto ExtractValue(RawValueType<MySQLCachePolicy>&& raw) {
  if constexpr (kHasRawValueType<MySQLCachePolicy>) {
    return Convert(std::move(raw),
                   formats::parse::To<ValueType<MySQLCachePolicy>>{});
  } else {
    return std::move(raw);
  }
}

// Component name in policy
template <typename T>
using HasNameImpl = std::enable_if_t<!std::string_view{T::kName}.empty()>;
template <typename T>
inline constexpr bool kHasName = meta::kIsDetected<HasNameImpl, T>;

// Component query in policy
template <typename T>
using HasQueryImpl = decltype(T::kQuery);
template <typename T>
inline constexpr bool kHasQuery = meta::kIsDetected<HasQueryImpl, T>;

// Component GetQuery in policy
template <typename T>
using HasGetQueryImpl = decltype(T::GetQuery());
template <typename T>
inline constexpr bool kHasGetQuery = meta::kIsDetected<HasGetQueryImpl, T>;

// Component kWhere in policy
template <typename T>
using HasWhere = decltype(T::kWhere);
template <typename T>
inline constexpr bool kHasWhere = meta::kIsDetected<HasWhere, T>;

// Updated field
template <typename T>
using HasUpdatedField = decltype(T::kUpdatedField);
template <typename T>
inline constexpr bool kHasUpdatedField = meta::kIsDetected<HasUpdatedField, T>;

template <typename T>
using WantsIncrementalUpdates =
    std::enable_if_t<!std::string_view{T::kUpdatedField}.empty()>;
template <typename T>
inline constexpr bool kWantsIncrementalUpdates =
    meta::kIsDetected<WantsIncrementalUpdates, T>;

// Key member in policy
template <typename T>
using KeyMemberTypeImpl =
    std::decay_t<std::invoke_result_t<decltype(T::KeyMember), ValueType<T>>>;
template <typename T>
inline constexpr bool kHasKeyMember = meta::kIsDetected<KeyMemberTypeImpl, T>;
template <typename T>
using KeyMemberType = meta::DetectedType<KeyMemberTypeImpl, T>;

// Data container for cache
template <typename T, typename = USERVER_NAMESPACE::utils::void_t<>>
struct DataCacheContainer final {
  static_assert(meta::kIsStdHashable<KeyMemberType<T>>,
                "With default CacheContainer, key type must be std::hash-able");

  using type = std::unordered_map<KeyMemberType<T>, ValueType<T>>;
};

template <typename T>
struct DataCacheContainer<
    T, USERVER_NAMESPACE::utils::void_t<typename T::CacheContainer>>
    final {
  using type = typename T::CacheContainer;
};

template <typename T>
using DataCacheContainerType = typename DataCacheContainer<T>::type;

// We have to whitelist container types, for which we perform by-element
// copying, because it's not correct for certain custom containers.
template <typename T>
inline constexpr bool kIsContainerCopiedByElement =
    meta::kIsInstantiationOf<std::unordered_map, T> ||
    meta::kIsInstantiationOf<std::map, T>;

template <typename T>
std::unique_ptr<T> CopyContainer(
    const T& container, [[maybe_unused]] std::size_t cpu_relax_iterations,
    tracing::ScopeTime& scope) {
  if constexpr (kIsContainerCopiedByElement<T>) {
    auto copy = std::make_unique<T>();
    if constexpr (meta::kIsReservable<T>) {
      copy->reserve(container.size());
    }

    utils::CpuRelax relax{cpu_relax_iterations, &scope};
    for (const auto& kv : container) {
      relax.Relax();
      copy->insert(kv);
    }
    return copy;
  } else {
    return std::make_unique<T>(container);
  }
}

template <typename Container, typename Value, typename KeyMember,
          typename... Args>
void CacheInsertOrAssign(Container& container, Value&& value,
                         const KeyMember& key_member, Args&&... /* args */) {
  // Args are only used to de-prioritize this default overload.
  static_assert(sizeof...(Args) == 0);
  // Copy 'key' to avoid aliasing issues in 'insert_or_assign'.
  auto key = std::invoke(key_member, value);
  container.insert_or_assign(std::move(key), std::forward<Value>(value));
}

template <typename T>
using HasOnWritesDoneImpl = decltype(std::declval<T&>().OnWritesDone());

template <typename T>
void OnWritesDone(T& container) {
  if constexpr (meta::kIsDetected<HasOnWritesDoneImpl, T>) {
    container.OnWritesDone();
  }
}

template <typename T>
using UpdatedFieldTypeImpl = typename T::UpdatedFieldType;
template <typename T>
inline constexpr bool kHasUpdatedFieldType =
    meta::kIsDetected<UpdatedFieldTypeImpl, T>;
template <typename T>
using UpdatedFieldType = meta::DetectedOr<std::chrono::system_clock::time_point,
                                          UpdatedFieldTypeImpl, T>;

template <typename T>
constexpr bool CheckUpdatedFieldType() {
  if constexpr (kHasUpdatedFieldType<T>) {
    static_assert(std::is_same_v<typename T::UpdatedFieldType,
                                 std::chrono::system_clock::time_point>,
                  "Invalid UpdatedFieldType, must be "
                  "std::chrono::system_clock::timepoint");
  } else {
    static_assert(!kWantsIncrementalUpdates<T>,
                  "UpdatedFieldType must be explicitly specified when using "
                  "incremental updates");
  }

  return true;
}

// Cluster host type policy
template <typename T>
using HasClusterHostTypeImpl = decltype(T::kClusterHostType);

template <typename T>
constexpr storages::mysql::ClusterHostType ClusterHostType() {
  if constexpr (meta::kIsDetected<HasClusterHostTypeImpl, T>) {
    return T::kClusterHostType;
  } else {
    return storages::mysql::ClusterHostType::kSlave;
  }
}

template <typename MySQLCachePolicy>
struct PolicyChecker {
  // Static assertions for cache traits
  static_assert(kHasName<MySQLCachePolicy>,
                "The MySQL cache policy must contain a static member `kName`");
  static_assert(kHasValueType<MySQLCachePolicy>,
                "The MySQL cache policy must define a type alias `ValueType`");
  static_assert(
      kHasKeyMember<MySQLCachePolicy>,
      "The MySQL cache policy must contain a static member `kKeyMember` "
      "with a pointer to a data or a function member with the object's key");
  static_assert(kHasQuery<MySQLCachePolicy> || kHasGetQuery<MySQLCachePolicy>,
                "The MySQL cache policy must contain a static data member "
                "`kQuery` with a select statement or a static member function "
                "`GetQuery` returning the query");
  static_assert(!(kHasQuery<MySQLCachePolicy> &&
                  kHasGetQuery<MySQLCachePolicy>),
                "The MySQL cache policy must define `kQuery` or "
                "`GetQuery`, not both");
  static_assert(
      kHasUpdatedField<MySQLCachePolicy>,
      "The MySQL cache policy must contain a static member "
      "`kUpdatedField`. If you don't want to use incremental updates, "
      "please set its value to `nullptr`");
  static_assert(CheckUpdatedFieldType<MySQLCachePolicy>());

  static std::string GetQuery() {
    if constexpr (kHasGetQuery<MySQLCachePolicy>) {
      return MySQLCachePolicy::GetQuery();
    } else {
      return std::string{MySQLCachePolicy::kQuery};
    }
  }

  using BaseType = components::CachingComponentBase<
      DataCacheContainerType<MySQLCachePolicy>>;
};

inline constexpr std::chrono::minutes kDefaultFullUpdateTimeout{1};
inline constexpr std::chrono::seconds kDefaultIncrementalUpdateTimeout{1};
inline constexpr std::chrono::milliseconds kStatementTimeoutOff{0};
inline constexpr std::chrono::milliseconds kCpuRelaxThreshold{10};
inline constexpr std::chrono::milliseconds kCpuRelaxInterval{2};

}  // namespace mysql_cache::impl

USERVER_NAMESPACE_END
