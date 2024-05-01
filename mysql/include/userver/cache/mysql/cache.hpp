#pragma once

/// @file userver/cache/mysql/cache.hpp
/// @brief @copybrief components::MySqlCache

#include <chrono>
#include <map>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <fmt/format.h>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/void_t.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/storages/mysql/cluster.hpp>
#include <userver/storages/mysql/component.hpp>
#include "userver/storages/mysql/dates.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @page Caching Component for MySQL
///
/// A typical components::MySqlCache usage consists of trait definition:
///
/// @snippet cache/postgres_cache_test.cpp Pg Cache Policy Trivial
///
/// and registration of the component in components::ComponentList:
///
/// @snippet cache/postgres_cache_test.cpp  Pg Cache Trivial Usage
///
/// See @ref scripts/docs/en/userver/caches.md for introduction into caches.
///
///
/// @section pg_cc_configuration Configuration
///
/// components::MySqlCache static configuration file should have a PostgreSQL
/// component name specified in `pgcomponent` configuration parameter.
///
/// Optionally the operation timeouts for cache loading can be specified.
///
/// ### Avoiding memory leaks
/// components::CachingComponentBase
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// full-update-op-timeout | timeout for a full update | 1m
/// incremental-update-op-timeout | timeout for an incremental update | 1s
/// update-correction | incremental update window adjustment | - (0 for caches with defined GetLastKnownUpdated)
/// chunk-size | number of rows to request from PostgreSQL via portals, 0 to fetch all rows in one request without portals | 1000
///
/// @section pg_cc_cache_policy Cache policy
///
/// Cache policy is the template argument of components::MySqlCache component.
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
/// ⇦ @ref scripts/docs/en/userver/cache_dumps.md | @ref scripts/docs/en/userver/lru_cache.md ⇨
/// @htmlonly </div> @endhtmlonly

// clang-format on

namespace mysql_cache::detail {

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

template <typename MySqlCachePolicy>
auto ExtractValue(RawValueType<MySqlCachePolicy>&& raw) {
  if constexpr (kHasRawValueType<MySqlCachePolicy>) {
    return Convert(std::move(raw),
                   formats::parse::To<ValueType<MySqlCachePolicy>>());
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
    meta::DetectedOr<storages::mysql::DateTime, UpdatedFieldTypeImpl, T>;

template <typename T>
constexpr bool CheckUpdatedFieldType() {
  if constexpr (kHasUpdatedFieldType<T>) {
    static_assert(
        std::is_same_v<typename T::UpdatedFieldType,
                       storages::mysql::DateTime> ||
            std::is_same_v<typename T::UpdatedFieldType,
                           storages::mysql::DateTime> ||
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
using HasClusterHostTypeImpl = decltype(T::ClusterHostType);

template <typename T>
constexpr storages::mysql::ClusterHostType ClusterHostType() {
  if constexpr (meta::kIsDetected<HasClusterHostTypeImpl, T>) {
    return T::ClusterHostType;
  } else {
    return storages::mysql::ClusterHostType::kPrimary;
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

template <typename MySqlCachePolicy>
struct PolicyChecker {
  // Static assertions for cache traits
  static_assert(
      kHasName<MySqlCachePolicy>,
      "The PosgreSQL cache policy must contain a static member `kName`");
  static_assert(
      kHasValueType<MySqlCachePolicy>,
      "The PosgreSQL cache policy must define a type alias `ValueType`");
  static_assert(
      kHasKeyMember<MySqlCachePolicy>,
      "The PostgreSQL cache policy must contain a static member `kKeyMember` "
      "with a pointer to a data or a function member with the object's key");
  static_assert(kHasQuery<MySqlCachePolicy> ||
                    kHasGetQuery<MySqlCachePolicy>,
                "The PosgreSQL cache policy must contain a static data member "
                "`kQuery` with a select statement or a static member function "
                "`GetQuery` returning the query");
  static_assert(!(kHasQuery<MySqlCachePolicy> &&
                  kHasGetQuery<MySqlCachePolicy>),
                "The PosgreSQL cache policy must define `kQuery` or "
                "`GetQuery`, not both");
  static_assert(
      kHasUpdatedField<MySqlCachePolicy>,
      "The PosgreSQL cache policy must contain a static member "
      "`kUpdatedField`. If you don't want to use incremental updates, "
      "please set its value to `nullptr`");
  static_assert(CheckUpdatedFieldType<MySqlCachePolicy>());

  /*
  static_assert(ClusterHostType<MySqlCachePolicy>() &
                    storages::postgres::kClusterHostRolesMask,
                "Cluster host role must be specified for caching component, "
                "please be more specific");
  */

  static storages::mysql::Query GetQuery() {
    if constexpr (kHasGetQuery<MySqlCachePolicy>) {
      return MySqlCachePolicy::GetQuery();
    } else {
      return MySqlCachePolicy::kQuery;
    }
  }

  using BaseType =
      CachingComponentBase<DataCacheContainerType<MySqlCachePolicy>>;
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
}  // namespace mysql_cache::detail

/// @ingroup userver_components
///
/// @brief Caching component for PostgreSQL. See @ref pg_cache.
///
/// @see @ref pg_cache, @ref scripts/docs/en/userver/caches.md
template <typename MySqlCachePolicy>
class MySqlCache final
    : public mysql_cache::detail::PolicyChecker<MySqlCachePolicy>::BaseType {
 public:
  // Type aliases
  using PolicyType = MySqlCachePolicy;
  using ValueType = mysql_cache::detail::ValueType<PolicyType>;
  using RawValueType = mysql_cache::detail::RawValueType<PolicyType>;
  using DataType = mysql_cache::detail::DataCacheContainerType<PolicyType>;
  using PolicyCheckerType = mysql_cache::detail::PolicyChecker<MySqlCachePolicy>;
  using UpdatedFieldType =
      mysql_cache::detail::UpdatedFieldType<MySqlCachePolicy>;
  using BaseType = typename PolicyCheckerType::BaseType;

  // Calculated constants
  constexpr static bool kIncrementalUpdates =
      mysql_cache::detail::kWantIncrementalUpdates<PolicyType>;
  constexpr static auto kClusterHostTypeFlags =
      mysql_cache::detail::ClusterHostType<PolicyType>();
  constexpr static auto kName = PolicyType::kName;

  MySqlCache(const ComponentConfig&, const ComponentContext&);
  ~MySqlCache() override;

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
  void CacheResults(std::vector<ValueType> res, CachedData& data_cache,
                    cache::UpdateStatisticsScope& stats_scope,
                    tracing::ScopeTime& scope);

  static storages::mysql::Query GetAllQuery();
  static storages::mysql::Query GetDeltaQuery();

  std::chrono::milliseconds ParseCorrection(const ComponentConfig& config);

  //std::vector<storages::postgres::ClusterPtr> clusters_;
  std::vector<std::shared_ptr<storages::mysql::Cluster>> clusters_;

  const std::chrono::system_clock::duration correction_;
  const std::chrono::milliseconds full_update_timeout_;
  const std::chrono::milliseconds incremental_update_timeout_;
  const std::size_t chunk_size_;
  std::size_t cpu_relax_iterations_parse_{0};
  std::size_t cpu_relax_iterations_copy_{0};
};

template <typename MySqlCachePolicy>
inline constexpr bool kHasValidate<MySqlCache<MySqlCachePolicy>> = true;

template <typename MySqlCachePolicy>
MySqlCache<MySqlCachePolicy>::MySqlCache(const ComponentConfig& config,
                                               const ComponentContext& context)
    : BaseType{config, context},
      correction_{ParseCorrection(config)},
      full_update_timeout_{
          config["full-update-op-timeout"].As<std::chrono::milliseconds>(
              mysql_cache::detail::kDefaultFullUpdateTimeout)},
      incremental_update_timeout_{
          config["incremental-update-op-timeout"].As<std::chrono::milliseconds>(
              mysql_cache::detail::kDefaultIncrementalUpdateTimeout)},
      chunk_size_{config["chunk-size"].As<size_t>(
          mysql_cache::detail::kDefaultChunkSize)} {
  /* TODO
  UINVARIANT(
      !chunk_size_ || storages::postgres::Portal::IsSupportedByDriver(),
      "Either set 'chunk-size' to 0, or enable PostgreSQL portals by building "
      "the framework with CMake option USERVER_FEATURE_PATCH_LIBPQ set to ON.");
  */
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
  /* TODO
  if (pg_alias.empty()) {
    throw storages::postgres::InvalidConfig{
        "No `pgcomponent` entry in configuration"};
  }
  */
  auto& pg_cluster_comp = context.FindComponent<userver::storages::mysql::Component>(pg_alias);
  const auto shard_count = 1;
  clusters_.resize(shard_count);
  for (size_t i = 0; i < shard_count; ++i) {
    clusters_[i] = pg_cluster_comp.GetCluster();
  }

  LOG_INFO() << "Cache " << kName << " full update query `"
             << GetAllQuery().GetStatement() << "` incremental update query `"
             << GetDeltaQuery().GetStatement() << "`";

  this->StartPeriodicUpdates();
}

template <typename MySqlCachePolicy>
MySqlCache<MySqlCachePolicy>::~MySqlCache() {
  this->StopPeriodicUpdates();
}

template <typename MySqlCachePolicy>
storages::mysql::Query MySqlCache<MySqlCachePolicy>::GetAllQuery() {
  storages::mysql::Query query = PolicyCheckerType::GetQuery();
  if constexpr (mysql_cache::detail::kHasWhere<MySqlCachePolicy>) {
    return {fmt::format("{} where {}", query.GetStatement(),
                        MySqlCachePolicy::kWhere),
            query.GetName()};
  } else {
    return query;
  }
}

template <typename MySqlCachePolicy>
storages::mysql::Query MySqlCache<MySqlCachePolicy>::GetDeltaQuery() {
  if constexpr (kIncrementalUpdates) {
    storages::mysql::Query query = PolicyCheckerType::GetQuery();
    if constexpr (mysql_cache::detail::kHasWhere<MySqlCachePolicy>) {
      return {
          fmt::format("{} where ({}) and {} >= $1", query.GetStatement(),
                      MySqlCachePolicy::kWhere, PolicyType::kUpdatedField),
          query.GetName()};
    } else {
      return {fmt::format("{} where {} >= $1", query.GetStatement(),
                          PolicyType::kUpdatedField),
              query.GetName()};
    }
  } else {
    return GetAllQuery();
  }
}

template <typename MySqlCachePolicy>
std::chrono::milliseconds MySqlCache<MySqlCachePolicy>::ParseCorrection(
    const ComponentConfig& config) {
  static constexpr std::string_view kUpdateCorrection = "update-correction";
  if (mysql_cache::detail::kHasCustomUpdated<MySqlCachePolicy> ||
      this->GetAllowedUpdateTypes() == cache::AllowedUpdateTypes::kOnlyFull) {
    return config[kUpdateCorrection].As<std::chrono::milliseconds>(0);
  } else {
    return config[kUpdateCorrection].As<std::chrono::milliseconds>();
  }
}

template <typename MySqlCachePolicy>
typename MySqlCache<MySqlCachePolicy>::UpdatedFieldType
MySqlCache<MySqlCachePolicy>::GetLastUpdated(
    [[maybe_unused]] std::chrono::system_clock::time_point last_update,
    const DataType& cache) const {
  if constexpr (mysql_cache::detail::kHasCustomUpdated<MySqlCachePolicy>) {
    return MySqlCachePolicy::GetLastKnownUpdated(cache);
  } else {
    return UpdatedFieldType{last_update - correction_};
  }
}

template <typename MySqlCachePolicy>
void MySqlCache<MySqlCachePolicy>::Update(
    cache::UpdateType type,
    const std::chrono::system_clock::time_point& /*last_update*/,
    const std::chrono::system_clock::time_point& /*now*/,
    cache::UpdateStatisticsScope& stats_scope) {
  if constexpr (!kIncrementalUpdates) {
    type = cache::UpdateType::kFull;
  }
  const auto query =
      (type == cache::UpdateType::kFull) ? GetAllQuery() : GetDeltaQuery();
  /* todo
  const std::chrono::milliseconds timeout = (type == cache::UpdateType::kFull)
                                                ? full_update_timeout_
                                                : incremental_update_timeout_;
  */
  // COPY current cached data
  auto scope = tracing::Span::CurrentSpan().CreateScopeTime(
      std::string{mysql_cache::detail::kCopyStage});
  auto data_cache = GetDataSnapshot(type, scope);
  [[maybe_unused]] const auto old_size = data_cache->size();

  scope.Reset(std::string{mysql_cache::detail::kFetchStage});

  size_t changes = 0;
  // Iterate clusters
  // TODO
  for (const std::shared_ptr<storages::mysql::Cluster>& cluster : clusters_) {
    if (chunk_size_ > 0) {
      /*auto trx = cluster->Begin(kClusterHostTypeFlags);
      auto portal =
          trx.MakePortal(query, GetLastUpdated(last_update, *data_cache));
      while (portal) {
        scope.Reset(std::string{mysql_cache::detail::kFetchStage});
        auto res = portal.Fetch(chunk_size_);
        stats_scope.IncreaseDocumentsReadCount(res.Size());

        scope.Reset(std::string{mysql_cache::detail::kParseStage});
        CacheResults(res, data_cache, stats_scope, scope);
        changes += res.Size();
      }
      trx.Commit();*/
    } else {
      //bool has_parameter = query.GetStatement().find('$') != std::string::npos;
      auto resultValues = cluster->Execute(userver::storages::mysql::ClusterHostType::kPrimary, query.GetStatement()).AsVector<ValueType>();
      stats_scope.IncreaseDocumentsReadCount(resultValues.size());

      scope.Reset(std::string{mysql_cache::detail::kParseStage});
      CacheResults(resultValues, data_cache, stats_scope, scope);
      changes += resultValues.size();
    }
  }

  scope.Reset();

  if constexpr (mysql_cache::detail::kIsContainerCopiedByElement<DataType>) {
    if (old_size > 0) {
      const auto elapsed_copy =
          scope.ElapsedTotal(std::string{mysql_cache::detail::kCopyStage});
      if (elapsed_copy > mysql_cache::detail::kCpuRelaxThreshold) {
        cpu_relax_iterations_copy_ = static_cast<std::size_t>(
            static_cast<double>(old_size) /
            (elapsed_copy / mysql_cache::detail::kCpuRelaxInterval));
        LOG_TRACE() << "Elapsed time for copying " << kName << " "
                    << elapsed_copy.count() << " for " << changes
                    << " data items is over threshold. Will relax CPU every "
                    << cpu_relax_iterations_parse_ << " iterations";
      }
    }
  }

  if (changes > 0) {
    const auto elapsed_parse =
        scope.ElapsedTotal(std::string{mysql_cache::detail::kParseStage});
    if (elapsed_parse > mysql_cache::detail::kCpuRelaxThreshold) {
      cpu_relax_iterations_parse_ = static_cast<std::size_t>(
          static_cast<double>(changes) /
          (elapsed_parse / mysql_cache::detail::kCpuRelaxInterval));
      LOG_TRACE() << "Elapsed time for parsing " << kName << " "
                  << elapsed_parse.count() << " for " << changes
                  << " data items is over threshold. Will relax CPU every "
                  << cpu_relax_iterations_parse_ << " iterations";
    }
  }
  if (changes > 0 || type == cache::UpdateType::kFull) {
    // Set current cache
    stats_scope.Finish(data_cache->size());
    mysql_cache::detail::OnWritesDone(*data_cache);
    this->Set(std::move(data_cache));
  } else {
    stats_scope.FinishNoChanges();
  }
}

template <typename MySqlCachePolicy>
bool MySqlCache<MySqlCachePolicy>::MayReturnNull() const {
  return mysql_cache::detail::MayReturnNull<PolicyType>();
}

template <typename MySqlCachePolicy>
void MySqlCache<MySqlCachePolicy>::CacheResults(
    std::vector<ValueType> res, CachedData& data_cache,
    cache::UpdateStatisticsScope& stats_scope, tracing::ScopeTime& scope) {
  auto values = res;
  utils::CpuRelax relax{cpu_relax_iterations_parse_, &scope};
  for (auto p = values.begin(); p != values.end(); ++p) {
    relax.Relax();
    try {
      using mysql_cache::detail::CacheInsertOrAssign;
      CacheInsertOrAssign(
          *data_cache, *p,
          MySqlCachePolicy::kKeyMember);
    } catch (const std::exception& e) {
      stats_scope.IncreaseDocumentsParseFailures(1);
      LOG_ERROR() << "Error parsing data row in cache '" << kName << "' to '"
                  << compiler::GetTypeName<ValueType>() << "': " << e.what();
    }
  }
}

template <typename MySqlCachePolicy>
typename MySqlCache<MySqlCachePolicy>::CachedData
MySqlCache<MySqlCachePolicy>::GetDataSnapshot(cache::UpdateType type,
                                                  tracing::ScopeTime& scope) {
  if (type == cache::UpdateType::kIncremental) {
    auto data = this->Get();
    if (data) {
      return mysql_cache::detail::CopyContainer(*data, cpu_relax_iterations_copy_,
                                             scope);
    }
  }
  return std::make_unique<DataType>();
}

namespace impl {

}  // namespace impl

template <typename MySqlCachePolicy>
yaml_config::Schema MySqlCache<MySqlCachePolicy>::GetStaticConfigSchema() {
  using ParentType =
      typename mysql_cache::detail::PolicyChecker<MySqlCachePolicy>::BaseType;
  const static std::string schema = R"(
type: object
description: Caching component for MySQL derived from components::CachingComponentBase.
additionalProperties: false
properties:
    full-update-op-timeout:
        type: string
        description: timeout for a full update
        defaultDescription: 1m
    incremental-update-op-timeout:
        type: string
        description: timeout for an incremental update
        defaultDescription: 1s
    update-correction:
        type: string
        description: incremental update window adjustment
        defaultDescription: 0 for caches with defined GetLastKnownUpdated
    chunk-size:
        type: integer
        description: number of rows to request from PostgreSQL, 0 to fetch all rows in one request
        defaultDescription: 1000
    pgcomponent:
        type: string
        description: PostgreSQL component name
        defaultDescription: ""
)";
  return yaml_config::MergeSchemas<ParentType>(schema);
}

}  // namespace components

//  namespace utils::impl::projected_set {
//
//  template <typename Set, typename Value, typename KeyMember>
//  void CacheInsertOrAssign(Set& set, Value&& value,
//                         const KeyMember& /*key_member*/) {
//  DoInsert(set, std::forward<Value>(value));
//  }
//}  // namespace utils::impl::projected_set

USERVER_NAMESPACE_END
