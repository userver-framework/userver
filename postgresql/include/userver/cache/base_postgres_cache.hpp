#pragma once

/// @file userver/cache/base_postgres_cache.hpp
/// @brief @copybrief components::PostgreCache

#include <userver/cache/base_postgres_cache_fwd.hpp>

#include <chrono>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <fmt/format.h>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/void_t.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @page pg_cache Caching Component for PostgreSQL
///
/// A typical components::PostgreCache usage consists of trait definition:
///
/// @snippet cache/postgres_cache_test.cpp Pg Cache Policy Trivial
///
/// and registration of the component in components::ComponentList:
///
/// @snippet cache/postgres_cache_test.cpp  Pg Cache Trivial Usage
///
/// See @ref md_en_userver_caches for introduction into caches.
///
///
/// @section pg_cc_configuration Configuration
///
/// components::PostgreCache static configuration file should have a PostgreSQL
/// component name specified in `pgcomponent` configuration parameter.
///
/// Optionally the operation timeouts for cache loading can be specified.
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// full-update-op-timeout | timeout for a full update | 1m
/// incremental-update-op-timeout | timeout for an incremental update | 1s
/// update-correction | incremental update window adjustment | - (0 for caches with defined GetLastKnownUpdated)
/// chunk-size | number of rows to request from PostgreSQL, 0 to fetch all rows in one request | 1000
///
/// @section pg_cc_cache_policy Cache policy
///
/// Cache policy is the template argument of components::PostgreCache component.
/// Please see the following code snippet for documentation.
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
///
/// In case one provides a custom CacheContainer within Policy, it is notified
/// of Update completion via its public member function OnWritesDone, if any.
/// See the following code snippet for an example of usage:
///
/// @snippet cache/postgres_cache_test.cpp Pg Cache Policy Custom Container With Write Notification Example
///
/// @section pg_cc_forward_declaration Forward Declaration
///
/// To forward declare a cache you can forward declare a trait and
/// include userver/cache/base_postgres_cache_fwd.hpp header. It is also useful to
/// forward declare the cache value type.
///
/// @snippet cache/postgres_cache_test_fwd.hpp Pg Cache Fwd Example
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref md_en_userver_cache_dumps | @ref md_en_userver_lru_cache ⇨
/// @htmlonly </div> @endhtmlonly

// clang-format on

namespace pg_cache::detail {

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

// Update field
template <typename T>
using HasUpdatedField = decltype(T::kUpdatedField);
template <typename T>
inline constexpr bool kHasUpdatedField = meta::kIsDetected<HasUpdatedField, T>;

template <typename T>
using WantIncrementalUpdates =
    std::enable_if_t<!std::string_view{T::kUpdatedField}.empty()>;
template <typename T>
inline constexpr bool kWantIncrementalUpdates =
    meta::kIsDetected<WantIncrementalUpdates, T>;

// Key member in policy
template <typename T>
using KeyMemberTypeImpl =
    std::decay_t<std::invoke_result_t<decltype(T::kKeyMember), ValueType<T>>>;
template <typename T>
inline constexpr bool kHasKeyMember = meta::kIsDetected<KeyMemberTypeImpl, T>;
template <typename T>
using KeyMemberType = meta::DetectedType<KeyMemberTypeImpl, T>;

// Data container for cache
template <typename T, typename = USERVER_NAMESPACE::utils::void_t<>>
struct DataCacheContainer {
  static_assert(meta::kIsStdHashable<KeyMemberType<T>>,
                "With default CacheContainer, key type must be std::hash-able");

  using type = std::unordered_map<KeyMemberType<T>, ValueType<T>>;
};

template <typename T>
struct DataCacheContainer<
    T, USERVER_NAMESPACE::utils::void_t<typename T::CacheContainer>> {
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
                         const KeyMember& key_member, Args&&... /*args*/) {
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
using HasCustomUpdatedImpl =
    decltype(T::GetLastKnownUpdated(std::declval<DataCacheContainerType<T>>()));

template <typename T>
inline constexpr bool kHasCustomUpdated =
    meta::kIsDetected<HasCustomUpdatedImpl, T>;

template <typename T>
using UpdatedFieldTypeImpl = typename T::UpdatedFieldType;
template <typename T>
inline constexpr bool kHasUpdatedFieldType =
    meta::kIsDetected<UpdatedFieldTypeImpl, T>;
template <typename T>
using UpdatedFieldType =
    meta::DetectedOr<storages::postgres::TimePointTz, UpdatedFieldTypeImpl, T>;

template <typename T>
constexpr bool CheckUpdatedFieldType() {
  if constexpr (kHasUpdatedFieldType<T>) {
    static_assert(
        std::is_same_v<typename T::UpdatedFieldType,
                       storages::postgres::TimePointTz> ||
            std::is_same_v<typename T::UpdatedFieldType,
                           storages::postgres::TimePoint> ||
            kHasCustomUpdated<T>,
        "Invalid UpdatedFieldType, must be either TimePointTz or TimePoint");
  } else {
    static_assert(!kWantIncrementalUpdates<T>,
                  "UpdatedFieldType must be explicitly specified when using "
                  "incremental updates");
  }
  return true;
}

// Cluster host type policy
template <typename T>
using HasClusterHostTypeImpl = decltype(T::kClusterHostType);

template <typename T>
constexpr storages::postgres::ClusterHostTypeFlags ClusterHostType() {
  if constexpr (meta::kIsDetected<HasClusterHostTypeImpl, T>) {
    return T::kClusterHostType;
  } else {
    return storages::postgres::ClusterHostType::kSlave;
  }
}

// May return null policy
template <typename T>
using HasMayReturnNull = decltype(T::kMayReturnNull);

template <typename T>
constexpr bool MayReturnNull() {
  if constexpr (meta::kIsDetected<HasMayReturnNull, T>) {
    return T::kMayReturnNull;
  } else {
    return false;
  }
}

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
  static_assert(CheckUpdatedFieldType<PostgreCachePolicy>());

  static_assert(ClusterHostType<PostgreCachePolicy>() &
                    storages::postgres::kClusterHostRolesMask,
                "Cluster host role must be specified for caching component, "
                "please be more specific");

  static storages::postgres::Query GetQuery() {
    if constexpr (kHasGetQuery<PostgreCachePolicy>) {
      return PostgreCachePolicy::GetQuery();
    } else {
      return PostgreCachePolicy::kQuery;
    }
  }

  using BaseType =
      CachingComponentBase<DataCacheContainerType<PostgreCachePolicy>>;
};

inline constexpr std::chrono::minutes kDefaultFullUpdateTimeout{1};
inline constexpr std::chrono::seconds kDefaultIncrementalUpdateTimeout{1};
inline constexpr std::chrono::milliseconds kStatementTimeoutOff{0};
inline constexpr std::chrono::milliseconds kCpuRelaxThreshold{10};
inline constexpr std::chrono::milliseconds kCpuRelaxInterval{2};

inline constexpr std::string_view kCopyStage = "copy_data";
inline constexpr std::string_view kFetchStage = "fetch";
inline constexpr std::string_view kParseStage = "parse";

inline constexpr std::size_t kDefaultChunkSize = 1000;
}  // namespace pg_cache::detail

/// @ingroup userver_components
///
/// @brief Caching component for PostgreSQL. See @ref pg_cache.
///
/// @see @ref pg_cache, @ref md_en_userver_caches
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
      pg_cache::detail::ClusterHostType<PolicyType>();
  constexpr static auto kName = PolicyType::kName;

  PostgreCache(const ComponentConfig&, const ComponentContext&);
  ~PostgreCache() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  using CachedData = std::unique_ptr<DataType>;

  UpdatedFieldType GetLastUpdated(
      std::chrono::system_clock::time_point last_update,
      const DataType& cache) const;

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope& stats_scope) override;

  bool MayReturnNull() const override;

  CachedData GetDataSnapshot(cache::UpdateType type, tracing::ScopeTime& scope);
  void CacheResults(storages::postgres::ResultSet res, CachedData& data_cache,
                    cache::UpdateStatisticsScope& stats_scope,
                    tracing::ScopeTime& scope);

  static storages::postgres::Query GetAllQuery();
  static storages::postgres::Query GetDeltaQuery();

  std::chrono::milliseconds ParseCorrection(const ComponentConfig& config);

  std::vector<storages::postgres::ClusterPtr> clusters_;

  const std::chrono::system_clock::duration correction_;
  const std::chrono::milliseconds full_update_timeout_;
  const std::chrono::milliseconds incremental_update_timeout_;
  const std::size_t chunk_size_;
  std::size_t cpu_relax_iterations_parse_{0};
  std::size_t cpu_relax_iterations_copy_{0};
};

template <typename PostgreCachePolicy>
inline constexpr bool kHasValidate<PostgreCache<PostgreCachePolicy>> = true;

template <typename PostgreCachePolicy>
PostgreCache<PostgreCachePolicy>::PostgreCache(const ComponentConfig& config,
                                               const ComponentContext& context)
    : BaseType{config, context},
      correction_{ParseCorrection(config)},
      full_update_timeout_{
          config["full-update-op-timeout"].As<std::chrono::milliseconds>(
              pg_cache::detail::kDefaultFullUpdateTimeout)},
      incremental_update_timeout_{
          config["incremental-update-op-timeout"].As<std::chrono::milliseconds>(
              pg_cache::detail::kDefaultIncrementalUpdateTimeout)},
      chunk_size_{config["chunk-size"].As<size_t>(
          pg_cache::detail::kDefaultChunkSize)} {
  if (this->GetAllowedUpdateTypes() ==
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

  LOG_INFO() << "Cache " << kName << " full update query `"
             << GetAllQuery().Statement() << "` incremental update query `"
             << GetDeltaQuery().Statement() << "`";

  this->StartPeriodicUpdates();
}

template <typename PostgreCachePolicy>
PostgreCache<PostgreCachePolicy>::~PostgreCache() {
  this->StopPeriodicUpdates();
}

template <typename PostgreCachePolicy>
storages::postgres::Query PostgreCache<PostgreCachePolicy>::GetAllQuery() {
  storages::postgres::Query query = PolicyCheckerType::GetQuery();
  if constexpr (pg_cache::detail::kHasWhere<PostgreCachePolicy>) {
    return {fmt::format("{} where {}", query.Statement(),
                        PostgreCachePolicy::kWhere),
            query.GetName()};
  } else {
    return query;
  }
}

template <typename PostgreCachePolicy>
storages::postgres::Query PostgreCache<PostgreCachePolicy>::GetDeltaQuery() {
  if constexpr (kIncrementalUpdates) {
    storages::postgres::Query query = PolicyCheckerType::GetQuery();

    if constexpr (pg_cache::detail::kHasWhere<PostgreCachePolicy>) {
      return {
          fmt::format("{} where ({}) and {} >= $1", query.Statement(),
                      PostgreCachePolicy::kWhere, PolicyType::kUpdatedField),
          query.GetName()};
    } else {
      return {fmt::format("{} where {} >= $1", query.Statement(),
                          PolicyType::kUpdatedField),
              query.GetName()};
    }
  } else {
    return GetAllQuery();
  }
}

template <typename PostgreCachePolicy>
std::chrono::milliseconds PostgreCache<PostgreCachePolicy>::ParseCorrection(
    const ComponentConfig& config) {
  static constexpr std::string_view kUpdateCorrection = "update-correction";
  if (pg_cache::detail::kHasCustomUpdated<PostgreCachePolicy> ||
      this->GetAllowedUpdateTypes() == cache::AllowedUpdateTypes::kOnlyFull) {
    return config[kUpdateCorrection].As<std::chrono::milliseconds>(0);
  } else {
    return config[kUpdateCorrection].As<std::chrono::milliseconds>();
  }
}

template <typename PostgreCachePolicy>
typename PostgreCache<PostgreCachePolicy>::UpdatedFieldType
PostgreCache<PostgreCachePolicy>::GetLastUpdated(
    [[maybe_unused]] std::chrono::system_clock::time_point last_update,
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
  const auto query =
      (type == cache::UpdateType::kFull) ? GetAllQuery() : GetDeltaQuery();
  const std::chrono::milliseconds timeout = (type == cache::UpdateType::kFull)
                                                ? full_update_timeout_
                                                : incremental_update_timeout_;

  // COPY current cached data
  auto scope = tracing::Span::CurrentSpan().CreateScopeTime(
      std::string{pg_cache::detail::kCopyStage});
  auto data_cache = GetDataSnapshot(type, scope);
  [[maybe_unused]] const auto old_size = data_cache->size();

  scope.Reset(std::string{pg_cache::detail::kFetchStage});

  size_t changes = 0;
  // Iterate clusters
  for (auto& cluster : clusters_) {
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
      bool has_parameter = query.Statement().find('$') != std::string::npos;
      auto res = has_parameter
                     ? cluster->Execute(
                           kClusterHostTypeFlags,
                           pg::CommandControl{
                               timeout, pg_cache::detail::kStatementTimeoutOff},
                           query, GetLastUpdated(last_update, *data_cache))
                     : cluster->Execute(
                           kClusterHostTypeFlags,
                           pg::CommandControl{
                               timeout, pg_cache::detail::kStatementTimeoutOff},
                           query);
      stats_scope.IncreaseDocumentsReadCount(res.Size());

      scope.Reset(std::string{pg_cache::detail::kParseStage});
      CacheResults(res, data_cache, stats_scope, scope);
      changes += res.Size();
    }
  }

  scope.Reset();

  if constexpr (pg_cache::detail::kIsContainerCopiedByElement<DataType>) {
    if (old_size > 0) {
      const auto elapsed_copy =
          scope.ElapsedTotal(std::string{pg_cache::detail::kCopyStage});
      if (elapsed_copy > pg_cache::detail::kCpuRelaxThreshold) {
        cpu_relax_iterations_copy_ = static_cast<std::size_t>(
            static_cast<double>(old_size) /
            (elapsed_copy / pg_cache::detail::kCpuRelaxInterval));
        LOG_TRACE() << "Elapsed time for copying " << kName << " "
                    << elapsed_copy.count() << " for " << changes
                    << " data items is over threshold. Will relax CPU every "
                    << cpu_relax_iterations_parse_ << " iterations";
      }
    }
  }

  if (changes > 0) {
    const auto elapsed_parse =
        scope.ElapsedTotal(std::string{pg_cache::detail::kParseStage});
    if (elapsed_parse > pg_cache::detail::kCpuRelaxThreshold) {
      cpu_relax_iterations_parse_ = static_cast<std::size_t>(
          static_cast<double>(changes) /
          (elapsed_parse / pg_cache::detail::kCpuRelaxInterval));
      LOG_TRACE() << "Elapsed time for parsing " << kName << " "
                  << elapsed_parse.count() << " for " << changes
                  << " data items is over threshold. Will relax CPU every "
                  << cpu_relax_iterations_parse_ << " iterations";
    }
  }
  if (changes > 0 || type == cache::UpdateType::kFull) {
    // Set current cache
    stats_scope.Finish(data_cache->size());
    pg_cache::detail::OnWritesDone(*data_cache);
    this->Set(std::move(data_cache));
  } else {
    stats_scope.FinishNoChanges();
  }
}

template <typename PostgreCachePolicy>
bool PostgreCache<PostgreCachePolicy>::MayReturnNull() const {
  return pg_cache::detail::MayReturnNull<PolicyType>();
}

template <typename PostgreCachePolicy>
void PostgreCache<PostgreCachePolicy>::CacheResults(
    storages::postgres::ResultSet res, CachedData& data_cache,
    cache::UpdateStatisticsScope& stats_scope, tracing::ScopeTime& scope) {
  auto values = res.AsSetOf<RawValueType>(storages::postgres::kRowTag);
  utils::CpuRelax relax{cpu_relax_iterations_parse_, &scope};
  for (auto p = values.begin(); p != values.end(); ++p) {
    relax.Relax();
    try {
      using pg_cache::detail::CacheInsertOrAssign;
      CacheInsertOrAssign(
          *data_cache, pg_cache::detail::ExtractValue<PostgreCachePolicy>(*p),
          PostgreCachePolicy::kKeyMember);
    } catch (const std::exception& e) {
      stats_scope.IncreaseDocumentsParseFailures(1);
      LOG_ERROR() << "Error parsing data row in cache '" << kName << "' to '"
                  << compiler::GetTypeName<ValueType>() << "': " << e.what();
    }
  }
}

template <typename PostgreCachePolicy>
typename PostgreCache<PostgreCachePolicy>::CachedData
PostgreCache<PostgreCachePolicy>::GetDataSnapshot(cache::UpdateType type,
                                                  tracing::ScopeTime& scope) {
  if (type == cache::UpdateType::kIncremental) {
    auto data = this->Get();
    if (data) {
      return pg_cache::detail::CopyContainer(*data, cpu_relax_iterations_copy_,
                                             scope);
    }
  }
  return std::make_unique<DataType>();
}

namespace impl {

std::string GetPostgreCacheSchema();

}  // namespace impl

template <typename PostgreCachePolicy>
yaml_config::Schema PostgreCache<PostgreCachePolicy>::GetStaticConfigSchema() {
  using ParentType =
      typename pg_cache::detail::PolicyChecker<PostgreCachePolicy>::BaseType;
  return yaml_config::MergeSchemas<ParentType>(impl::GetPostgreCacheSchema());
}

}  // namespace components

namespace utils::impl::projected_set {

template <typename Set, typename Value, typename KeyMember>
void CacheInsertOrAssign(Set& set, Value&& value,
                         const KeyMember& /*key_member*/) {
  DoInsert(set, std::forward<Value>(value));
}

}  // namespace utils::impl::projected_set

USERVER_NAMESPACE_END
