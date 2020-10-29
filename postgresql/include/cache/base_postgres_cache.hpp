#pragma once

/// @file cache/base_postgres_cache.hpp
/// @brief Caching Component for PostgreSQL

#include <chrono>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <cache/cache_statistics.hpp>
#include <cache/caching_component_base.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/component.hpp>
#include <storages/postgres/io/chrono.hpp>

#include <utils/algo.hpp>
#include <utils/assert.hpp>
#include <utils/cpu_relax.hpp>
#include <utils/meta.hpp>
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

template <typename PostgreCachePolicy>
auto ExtractValue(RawValueType<PostgreCachePolicy>&& raw) {
  if constexpr (kHasRawValueType<PostgreCachePolicy>) {
    return Convert(std::move(raw),
                   formats::parse::To<ValueType<PostgreCachePolicy>>());
  } else {
    return std::move(raw);
  }
}

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

// Component kWhere in policy
template <typename T, typename = ::utils::void_t<>>
struct HasWhere : std::false_type {};
template <typename T>
struct HasWhere<T, ::utils::void_t<decltype(T::kWhere)>> : std::true_type {};
template <typename T>
constexpr bool kHasWhere = HasWhere<T>::value;

// Update field
template <typename T, typename = ::utils::void_t<>>
struct HasUpdatedField : std::false_type {};
template <typename T>
struct HasUpdatedField<T, ::utils::void_t<decltype(T::kUpdatedField)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasUpdatedField = HasUpdatedField<T>::value;

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
struct HasKeyMember<T, ::utils::void_t<decltype(std::invoke(
                           T::kKeyMember, std::declval<ValueType<T>>()))>>
    : std::true_type {};
template <typename T>
constexpr bool kHasKeyMember = HasKeyMember<T>::value;

template <typename T>
auto GetKeyValue(const ValueType<T>& v) {
  return std::invoke(T::kKeyMember, v);
}

template <typename T>
struct KeyMemberTypeImpl {
  using type =
      std::decay_t<decltype(GetKeyValue<T>(std::declval<ValueType<T>>()))>;
};
template <typename T>
using KeyMemberType = typename KeyMemberTypeImpl<T>::type;

// Data container for cache
template <typename T, typename = ::utils::void_t<>>
struct DataCacheContainer {
  static_assert(meta::kIsStdHashable<KeyMemberType<T>>,
                "With default CacheContainer, key type must be std::hash-able");

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

// Update field type
template <typename T, typename = ::utils::void_t<>>
struct HasUpdatedFieldType : std::false_type {
  static_assert(!kWantIncrementalUpdates<T>,
                "UpdatedFieldType must be explicitly specified when using "
                "incremental updates");
};
template <typename T>
struct HasUpdatedFieldType<T, ::utils::void_t<typename T::UpdatedFieldType>>
    : std::true_type {
  static_assert(
      std::is_same_v<typename T::UpdatedFieldType,
                     storages::postgres::TimePointTz> ||
          std::is_same_v<typename T::UpdatedFieldType,
                         storages::postgres::TimePoint> ||
          kHasCustomUpdated<T>,
      "Invalid UpdatedFieldType, must be either TimePointTz or TimePoint");
};
template <typename T>
constexpr bool kHasUpdatedFieldType = HasUpdatedFieldType<T>::value;

template <typename T, bool HasUpdatedFieldType>
struct UpdatedFieldTypeImpl {
  using Type = typename T::UpdatedFieldType;
};
template <typename T>
struct UpdatedFieldTypeImpl<T, false> {
  using Type = storages::postgres::TimePointTz;
};
template <typename T>
using UpdatedFieldType =
    typename UpdatedFieldTypeImpl<T, kHasUpdatedFieldType<T>>::Type;

// Cluster host type policy
template <typename T, typename = ::utils::void_t<>>
struct PostgresClusterHostTypeFlags {
  constexpr static storages::postgres::ClusterHostTypeFlags value{
      storages::postgres::ClusterHostType::kSlave};
};

template <typename T>
struct PostgresClusterHostTypeFlags<
    T, ::utils::void_t<decltype(T::kClusterHostType)>> {
  constexpr static storages::postgres::ClusterHostTypeFlags value{
      T::kClusterHostType};
};

template <typename T>
constexpr auto kPostgresClusterHostTypeFlags =
    PostgresClusterHostTypeFlags<T>::value;

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
      kHasUpdatedField<PostgreCachePolicy>,
      "The PosgreSQL cache policy must contain a static member "
      "`kUpdatedField`. If you don't want to use incremental updates, "
      "please set its value to `nullptr`");

  static_assert(kPostgresClusterHostTypeFlags<PostgreCachePolicy> &
                    storages::postgres::kClusterHostRolesMask,
                "Cluster host role must be specified for caching component, "
                "please be more specific");

  static std::string GetQuery() {
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
constexpr TimeStorage::RealMilliseconds kCpuRelaxThreshold{10};
constexpr TimeStorage::RealMilliseconds kCpuRelaxInterval{2};

constexpr std::string_view kCopyStage = "copy_data";
constexpr std::string_view kFetchStage = "fetch";
constexpr std::string_view kParseStage = "parse";
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
  using UpdatedFieldType =
      pg_cache::detail::UpdatedFieldType<PostgreCachePolicy>;
  using BaseType = typename PolicyCheckerType::BaseType;

  // Calculated constants
  constexpr static bool kIncrementalUpdates =
      pg_cache::detail::kWantIncrementalUpdates<PolicyType>;
  constexpr static auto kClusterHostTypeFlags =
      pg_cache::detail::kPostgresClusterHostTypeFlags<PolicyType>;
  constexpr static auto kName = PolicyType::kName;

  PostgreCache(const ComponentConfig&, const ComponentContext&);
  ~PostgreCache();

 private:
  using CachedData = std::unique_ptr<DataType>;

  UpdatedFieldType GetLastUpdated(
      std::chrono::system_clock::time_point last_update,
      const DataType& cache) const;

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope& stats_scope) override;

  CachedData GetDataSnapshot(cache::UpdateType type);
  void CacheResults(storages::postgres::ResultSet res, CachedData& data_cache,
                    cache::UpdateStatisticsScope& stats_scope,
                    ScopeTime& scope);

  static std::string GetAllQuery();
  static std::string GetDeltaQuery();

  std::vector<storages::postgres::ClusterPtr> clusters_;

  const std::chrono::system_clock::duration correction_;
  const std::chrono::milliseconds full_update_timeout_;
  const std::chrono::milliseconds incremental_update_timeout_;
  const std::size_t chunk_size_;
  std::size_t cpu_relax_iterations_{0};
};

template <typename PostgreCachePolicy>
PostgreCache<PostgreCachePolicy>::PostgreCache(const ComponentConfig& config,
                                               const ComponentContext& context)
    : BaseType{config, context, kName},
      correction_{config["update-correction"].As<std::chrono::milliseconds>(0)},
      full_update_timeout_{
          config["full-update-op-timeout"].As<std::chrono::milliseconds>(
              pg_cache::detail::kDefaultFullUpdateTimeout)},
      incremental_update_timeout_{
          config["incremental-update-op-timeout"].As<std::chrono::milliseconds>(
              pg_cache::detail::kDefaultIncrementalUpdateTimeout)},
      chunk_size_{config["chunk-size"].As<size_t>(0)} {
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

  const auto pg_alias = config["pgcomponent"].As<std::string>("");
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
  if constexpr (pg_cache::detail::kHasWhere<PostgreCachePolicy>) {
    return fmt::format("{} where {}", query, PostgreCachePolicy::kWhere);
  } else {
    return query;
  }
}

template <typename PostgreCachePolicy>
std::string PostgreCache<PostgreCachePolicy>::GetDeltaQuery() {
  if constexpr (kIncrementalUpdates) {
    std::string query = PolicyCheckerType::GetQuery();

    if constexpr (pg_cache::detail::kHasWhere<PostgreCachePolicy>) {
      return fmt::format("{} where ({}) and {} >= $1", query,
                         PostgreCachePolicy::kWhere, PolicyType::kUpdatedField);
    } else {
      return fmt::format("{} where {} >= $1", query, PolicyType::kUpdatedField);
    }
  } else {
    return GetAllQuery();
  }
}

template <typename PostgreCachePolicy>
typename PostgreCache<PostgreCachePolicy>::UpdatedFieldType
PostgreCache<PostgreCachePolicy>::GetLastUpdated(
    std::chrono::system_clock::time_point last_update,
    const DataType& cache) const {
  if constexpr (pg_cache::detail::kHasCustomUpdated<PostgreCachePolicy>) {
    return PostgreCachePolicy::GetLastKnownUpdated(cache);
  } else {
    return UpdatedFieldType{last_update - correction_};
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
  auto scope = tracing::Span::CurrentSpan().CreateScopeTime(
      std::string{pg_cache::detail::kCopyStage});
  auto data_cache = GetDataSnapshot(type);

  scope.Reset(std::string{pg_cache::detail::kFetchStage});

  size_t changes = 0;
  // Iterate clusters
  for (auto cluster : clusters_) {
    if (chunk_size_ > 0) {
      auto trx = cluster->Begin(
          kClusterHostTypeFlags, pg::Transaction::RO,
          pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff});
      auto portal =
          trx.MakePortal(query, GetLastUpdated(last_update, *data_cache));
      while (portal) {
        scope.Reset(std::string{pg_cache::detail::kFetchStage});
        auto res = portal.Fetch(chunk_size_);
        stats_scope.IncreaseDocumentsReadCount(res.Size());

        scope.Reset(std::string{pg_cache::detail::kParseStage});
        CacheResults(res, data_cache, stats_scope, scope);
        changes += res.Size();
      }
      trx.Commit();
    } else {
      auto res = cluster->Execute(
          kClusterHostTypeFlags,
          pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff},
          query, GetLastUpdated(last_update, *data_cache));
      stats_scope.IncreaseDocumentsReadCount(res.Size());

      scope.Reset(std::string{pg_cache::detail::kParseStage});
      CacheResults(res, data_cache, stats_scope, scope);
      changes += res.Size();
    }
  }

  scope.Reset();

  if (changes > 0) {
    auto elapsed =
        scope.ElapsedTotal(std::string{pg_cache::detail::kParseStage});
    if (elapsed > pg_cache::detail::kCpuRelaxThreshold) {
      cpu_relax_iterations_ = static_cast<std::size_t>(
          static_cast<double>(changes) /
          (elapsed / pg_cache::detail::kCpuRelaxInterval));
      LOG_TRACE() << "Elapsed time for " << kName << " " << elapsed.count()
                  << " for " << changes
                  << " data items is over threshold. Will relax CPU every "
                  << cpu_relax_iterations_ << " iterations";
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
    cache::UpdateStatisticsScope& stats_scope, ScopeTime& scope) {
  auto values = res.AsSetOf<RawValueType>(storages::postgres::kRowTag);
  utils::CpuRelax relax{cpu_relax_iterations_};
  for (auto p = values.begin(); p != values.end(); ++p) {
    relax.Relax(scope);
    try {
      auto value = pg_cache::detail::ExtractValue<PostgreCachePolicy>(*p);
      auto key = pg_cache::detail::GetKeyValue<PolicyType>(value);
      data_cache->insert_or_assign(std::move(key), std::move(value));
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
