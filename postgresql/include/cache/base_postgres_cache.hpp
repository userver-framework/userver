#pragma once

/// @file cache/base_postgres_cache.hpp
/// @brief Caching Component for PostgreSQL

#include <chrono>
#include <type_traits>
#include <unordered_map>

#include <cache/cache_statistics.hpp>
#include <cache/caching_component_base.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/component.hpp>

#include <utils/algo.hpp>
#include <utils/assert.hpp>
#include <utils/void_t.hpp>

#include <compiler/demangle.hpp>
#include <logging/log.hpp>

namespace components {

// clang-format off

/// @page pg_cache Caching Component for PostgreSQL
///
/// @par Configuration
///
/// PostgreSQL component name must be specified in `pgcomponent` configuration
/// parameter.
///
/// Optionally the operation timeouts for cache loading can be specified:
/// * full-update-op-timeout - timeout for a full update,
///                            default value 1 minute
/// * incremental-update-op-timeout - timeout for an incremental update,
///                                   default value 1 second
/// * update-correction - incremental update window adjustment,
///                       default value 0
/// * chunk-size - number of rows to request from PostgreSQL. default value 0,
///                all rows are fetched in one request.
///
/// @par Cache policy
///
/// Cache policy is the template argument of component. Please see the following
/// code snippet for documentation.
///
/// @snippet cache/postgres_cache_test.cpp Pg Cache Policy Example
///
/// The query can be a std::string. But due to non-guaranteed order of static
/// data members initialization, std::string should be returned from a static
/// member function, please see the following code snippet.
///
/// @snippet cache/postgres_cache_test.cpp Pg Cache Policy GetQuery Example
///
/// Policy may have static function GetLastKnownUpdated. It should be used
/// when new entries from database are taken via revision, identifier, or
/// anything else, but not timestamp of the last update.
/// If this function is supplied, new entries are taken from db with condition
/// 'WHERE kUpdatedField > GetLastKnownUpdated(cache_container)'.
/// Otherwise, condition is
/// 'WHERE kUpdatedField > last_update - correction_'.
/// See the following code snippet for an example of usage
///
/// @snippet cache/postgres_cache_test.cpp Pg Cache Policy Custom Updated Example

// clang-format on

namespace pg_cache::detail {

template <typename T>
using ValueType = typename T::ValueType;

template <typename T, typename = ::utils::void_t<>>
struct HasValueType : std::false_type {};
template <typename T>
struct HasValueType<T, ::utils::void_t<typename T::ValueType>>
    : std::true_type {};
template <typename T>
constexpr bool kHasValueType = HasValueType<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasRawValueType : std::false_type {};
template <typename T>
struct HasRawValueType<T, ::utils::void_t<typename T::RawValueType>>
    : std::true_type {};
template <typename T>
constexpr bool kHasRawValueType = HasRawValueType<T>::value;

template <typename T, bool HasRawValueType>
struct RawValueTypeImpl {
  using Type = typename T::RawValueType;
};
template <typename T>
struct RawValueTypeImpl<T, false> {
  using Type = typename T::ValueType;
};
template <typename T>
using RawValueType = typename RawValueTypeImpl<T, kHasRawValueType<T>>::Type;

// Component name in policy
template <typename T, typename = ::utils::void_t<>>
struct HasName : std::false_type {};
template <typename T>
struct HasName<T, ::utils::void_t<decltype(T::kName)>>
    : std::integral_constant<bool, (T::kName != nullptr) &&
                                       (::utils::StrLen(T::kName) > 0)> {};
template <typename T>
constexpr bool kHasName = HasName<T>::value;

// Component query in policy
template <typename T, typename = ::utils::void_t<>>
struct HasQuery : std::false_type {};
template <typename T>
struct HasQuery<T, ::utils::void_t<decltype(T::kQuery)>> : std::true_type {};
template <typename T>
constexpr bool kHasQuery = HasQuery<T>::value;

// Component GetQuery in policy
template <typename T, typename = ::utils::void_t<>>
struct HasGetQuery : std::false_type {};
template <typename T>
struct HasGetQuery<T, ::utils::void_t<decltype(T::GetQuery())>>
    : std::true_type {};
template <typename T>
constexpr bool kHasGetQuery = HasGetQuery<T>::value;

// Update field
template <typename T, typename = ::utils::void_t<>>
struct HasUpdateField : std::false_type {};
template <typename T>
struct HasUpdateField<T, ::utils::void_t<decltype(T::kUpdatedField)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasUpdateField = HasUpdateField<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct WantIncrementalUpdates : std::false_type {};
template <typename T>
struct WantIncrementalUpdates<T, ::utils::void_t<decltype(T::kUpdatedField)>>
    : std::integral_constant<bool,
                             (T::kUpdatedField != nullptr) &&
                                 (::utils::StrLen(T::kUpdatedField) > 0)> {};
template <typename T>
constexpr bool kWantIncrementalUpdates = WantIncrementalUpdates<T>::value;

// Key member in policy
template <typename T, typename = ::utils::void_t<>>
struct HasKeyMember : std::false_type {};
template <typename T>
struct HasKeyMember<T, ::utils::void_t<decltype(T::kKeyMember)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasKeyMember = HasKeyMember<T>::value;

template <typename T>
constexpr auto GetKeyValue(const ValueType<T>& v) {
  if constexpr (std::is_member_function_pointer<decltype(T::kKeyMember)>{}) {
    return (v.*T::kKeyMember)();
  } else {
    return v.*T::kKeyMember;
  }
}

template <typename T>
struct KeyMember {
  using type =
      std::decay_t<decltype(GetKeyValue<T>(std::declval<ValueType<T>>()))>;
};
template <typename T>
using KeyMemberType = typename KeyMember<T>::type;

// Data container for cache
template <typename T, typename = ::utils::void_t<>>
struct DataCacheContainer {
  using type = std::unordered_map<KeyMemberType<T>, ValueType<T>>;
};

template <typename T>
struct DataCacheContainer<T, ::utils::void_t<typename T::CacheContainer>> {
  using type = typename T::CacheContainer;
  // TODO Checks that the type is some sort of a map
};

template <typename T>
using DataCacheContainerType = typename DataCacheContainer<T>::type;

template <typename T, typename = utils::void_t<>>
struct HasCustomUpdated : std::false_type {};

template <typename T>
struct HasCustomUpdated<T, utils::void_t<decltype(T::GetLastKnownUpdated(
                               std::declval<DataCacheContainerType<T>>()))>>
    : std::true_type {};

template <typename T>
constexpr bool kHasCustomUpdated = HasCustomUpdated<T>::value;

// Cluster host type policy
template <typename T, typename = ::utils::void_t<>>
struct PostgresClusterType
    : std::integral_constant<storages::postgres::ClusterHostType,
                             storages::postgres::ClusterHostType::kSlave> {};
template <typename T>
struct PostgresClusterType<T, ::utils::void_t<decltype(T::kClusterHostType)>>
    : std::integral_constant<storages::postgres::ClusterHostType,
                             T::kClusterHostType> {};

template <typename T>
constexpr storages::postgres::ClusterHostType kPostgresClusterType =
    PostgresClusterType<T>::value;

template <typename PostgreCachePolicy>
struct PolicyChecker {
  // Static assertions for cache traits
  static_assert(
      kHasName<PostgreCachePolicy>,
      "The PosgreSQL cache policy must contain a static member `kName`");
  static_assert(
      kHasValueType<PostgreCachePolicy>,
      "The PosgreSQL cache policy must define a type alias `ValueType`");
  static_assert(
      kHasKeyMember<PostgreCachePolicy>,
      "The PostgreSQL cache policy must contain a static member `kKeyMember` "
      "with a pointer to a data or a function member with the object's key");
  static_assert(kHasQuery<PostgreCachePolicy> ||
                    kHasGetQuery<PostgreCachePolicy>,
                "The PosgreSQL cache policy must contain a static data member "
                "`kQuery` with a select statement or a static member function "
                "`GetQuery` returning the query");
  static_assert(!(kHasQuery<PostgreCachePolicy> &&
                  kHasGetQuery<PostgreCachePolicy>),
                "The PosgreSQL cache policy must define `kQuery` or "
                "`GetQuery`, not both");
  static_assert(
      kHasUpdateField<PostgreCachePolicy>,
      "The PosgreSQL cache policy must contain a static member "
      "`kUpdatedField`. If you don't want to use incremental updates, "
      "please set its value to `nullptr`");

  static_assert(kPostgresClusterType<PostgreCachePolicy> !=
                    storages::postgres::ClusterHostType::kAny,
                "`Any` cluster host type cannot be used for caching component, "
                "please be more specific");

  static auto GetQuery() {
    if constexpr (kHasGetQuery<PostgreCachePolicy>) {
      return PostgreCachePolicy::GetQuery();
    } else {
      return PostgreCachePolicy::kQuery;
    }
  }

  using BaseType =
      CachingComponentBase<DataCacheContainerType<PostgreCachePolicy>>;
};

constexpr std::chrono::milliseconds kDefaultFullUpdateTimeout =
    std::chrono::minutes{1};
constexpr std::chrono::milliseconds kDefaultIncrementalUpdateTimeout =
    std::chrono::seconds{1};
constexpr std::chrono::milliseconds kStatementTimeoutOff{0};

}  // namespace pg_cache::detail

template <typename PostgreCachePolicy>
class PostgreCache final
    : public pg_cache::detail::PolicyChecker<PostgreCachePolicy>::BaseType {
 public:
  // Type aliases
  using PolicyType = PostgreCachePolicy;
  using ValueType = pg_cache::detail::ValueType<PolicyType>;
  using RawValueType = pg_cache::detail::RawValueType<PolicyType>;
  using DataType = pg_cache::detail::DataCacheContainerType<PolicyType>;
  using PolicyCheckerType = pg_cache::detail::PolicyChecker<PostgreCachePolicy>;
  using BaseType = typename PolicyCheckerType::BaseType;

  // Calculated constants
  constexpr static bool kIncrementalUpdates =
      pg_cache::detail::kWantIncrementalUpdates<PolicyType>;
  constexpr static storages::postgres::ClusterHostType kClusterHostType =
      pg_cache::detail::kPostgresClusterType<PolicyType>;
  constexpr static auto kName = PolicyType::kName;

  PostgreCache(const ComponentConfig&, const ComponentContext&);
  ~PostgreCache();

 private:
  using CachedData = std::unique_ptr<DataType>;

  auto GetLastUpdated(std::chrono::system_clock::time_point last_update,
                      const DataType& cache) const;

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope& stats_scope) override;

  CachedData GetDataSnapshot(cache::UpdateType type);
  void CacheResults(storages::postgres::ResultSet res, CachedData& data_cache,
                    cache::UpdateStatisticsScope& stats_scope);

  static std::string GetAllQuery();
  static std::string GetDeltaQuery();

  std::vector<storages::postgres::ClusterPtr> clusters_;

  const std::chrono::system_clock::duration correction_;
  const std::chrono::milliseconds full_update_timeout_;
  const std::chrono::milliseconds incremental_update_timeout_;
  const std::size_t chunk_size_;
};

template <typename PostgreCachePolicy>
PostgreCache<PostgreCachePolicy>::PostgreCache(const ComponentConfig& config,
                                               const ComponentContext& context)
    : BaseType{config, context, kName},
      correction_{config.ParseDuration("update-correction",
                                       std::chrono::milliseconds{0})},
      full_update_timeout_{
          config.ParseDuration("full-update-op-timeout",
                               pg_cache::detail::kDefaultFullUpdateTimeout)},
      incremental_update_timeout_{config.ParseDuration(
          "incremental-update-op-timeout",
          pg_cache::detail::kDefaultIncrementalUpdateTimeout)},
      chunk_size_{config.ParseUint64("chunk-size", 0)} {
  if (BaseType::AllowedUpdateTypes() ==
          cache::AllowedUpdateTypes::kFullAndIncremental &&
      !kIncrementalUpdates) {
    throw std::logic_error(
        "Incremental update support is requested in config but no update field "
        "name is specified in traits of '" +
        config.Name() + "' cache");
  }
  if (correction_.count() < 0) {
    throw std::logic_error(
        "Refusing to set forward (negative) update correction requested in "
        "config for '" +
        config.Name() + "' cache");
  }

  const auto pg_alias = config.ParseString("pgcomponent", {});
  if (pg_alias.empty()) {
    throw storages::postgres::InvalidConfig{
        "No `pgcomponent` entry in configuration"};
  }
  auto& pg_cluster_comp = context.FindComponent<components::Postgres>(pg_alias);
  const auto shard_count = pg_cluster_comp.GetShardCount();
  clusters_.resize(shard_count);
  for (size_t i = 0; i < shard_count; ++i) {
    clusters_[i] = pg_cluster_comp.GetClusterForShard(i);
  }

  LOG_INFO() << "Cache " << kName << " full update query `" << GetAllQuery()
             << "` incremental update query `" << GetDeltaQuery() << "`";

  this->StartPeriodicUpdates();
}

template <typename PostgreCachePolicy>
PostgreCache<PostgreCachePolicy>::~PostgreCache() {
  this->StopPeriodicUpdates();
}

template <typename PostgreCachePolicy>
std::string PostgreCache<PostgreCachePolicy>::GetAllQuery() {
  std::string query = PolicyCheckerType::GetQuery();
  UASSERT_MSG(!query.empty(), "Query body of cache policy must not be empty");
  return query;
}

template <typename PostgreCachePolicy>
std::string PostgreCache<PostgreCachePolicy>::GetDeltaQuery() {
  using namespace std::string_literals;
  const auto query = GetAllQuery();
  if constexpr (kIncrementalUpdates) {
    return query + " where "s + PolicyType::kUpdatedField + " >= $1";
  } else {
    return query;
  }
}

template <typename PostgreCachePolicy>
auto PostgreCache<PostgreCachePolicy>::GetLastUpdated(
    std::chrono::system_clock::time_point last_update,
    const DataType& cache) const {
  if constexpr (pg_cache::detail::kHasCustomUpdated<PostgreCachePolicy>) {
    return PostgreCachePolicy::GetLastKnownUpdated(cache);
  } else {
    return last_update - correction_;
  }
}

template <typename PostgreCachePolicy>
void PostgreCache<PostgreCachePolicy>::Update(
    cache::UpdateType type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& /*now*/,
    cache::UpdateStatisticsScope& stats_scope) {
  namespace pg = storages::postgres;
  if constexpr (!kIncrementalUpdates) {
    type = cache::UpdateType::kFull;
  }
  const std::string query =
      (type == cache::UpdateType::kFull) ? GetAllQuery() : GetDeltaQuery();
  const std::chrono::milliseconds timeout = (type == cache::UpdateType::kFull)
                                                ? full_update_timeout_
                                                : incremental_update_timeout_;

  // COPY current cached data
  auto data_cache = GetDataSnapshot(type);
  size_t changes = 0;
  // Iterate clusters
  for (auto cluster : clusters_) {
    if (chunk_size_ > 0) {
      auto trx = cluster->Begin(
          kClusterHostType, pg::Transaction::RO,
          pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff});
      auto portal =
          trx.MakePortal(query, GetLastUpdated(last_update, *data_cache));
      while (portal) {
        auto res = portal.Fetch(chunk_size_);
        stats_scope.IncreaseDocumentsReadCount(res.Size());
        CacheResults(res, data_cache, stats_scope);
        changes += res.Size();
      }
      trx.Commit();
    } else {
      auto res = cluster->Execute(
          kClusterHostType,
          pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff},
          query, GetLastUpdated(last_update, *data_cache));
      stats_scope.IncreaseDocumentsReadCount(res.Size());
      CacheResults(res, data_cache, stats_scope);
      changes += res.Size();
    }
  }
  if (changes > 0 || type == cache::UpdateType::kFull) {
    // Set current cache
    stats_scope.Finish(data_cache->size());
    this->Set(std::move(data_cache));
  } else {
    stats_scope.FinishNoChanges();
  }
}

template <typename PostgreCachePolicy>
void PostgreCache<PostgreCachePolicy>::CacheResults(
    storages::postgres::ResultSet res, CachedData& data_cache,
    cache::UpdateStatisticsScope& stats_scope) {
  auto values = res.AsSetOf<RawValueType>(storages::postgres::kRowTag);
  for (auto p = values.begin(); p != values.end(); ++p) {
    try {
      if constexpr (pg_cache::detail::kHasRawValueType<PolicyType>) {
        auto value = Convert(
            std::move(*p),
            formats::parse::To<pg_cache::detail::ValueType<PolicyType>>());
        auto key = pg_cache::detail::GetKeyValue<PolicyType>(value);
        data_cache->insert({std::move(key), std::move(value)});
      } else {
        auto value = *p;
        auto key = pg_cache::detail::GetKeyValue<PolicyType>(value);
        data_cache->insert({std::move(key), std::move(value)});
      }
    } catch (const std::exception& e) {
      stats_scope.IncreaseDocumentsParseFailures(1);
      LOG_ERROR() << "Error parsing data row in cache '" << kName << "' to '"
                  << compiler::GetTypeName<ValueType>() << "': " << e.what();
    }
  }
}

template <typename PostgreCachePolicy>
typename PostgreCache<PostgreCachePolicy>::CachedData
PostgreCache<PostgreCachePolicy>::GetDataSnapshot(cache::UpdateType type) {
  if (type == cache::UpdateType::kIncremental) {
    auto data = this->Get();
    if (data) {
      return std::make_unique<DataType>(*data);
    }
  }
  return std::make_unique<DataType>();
}
}  // namespace components
