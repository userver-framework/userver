#pragma once

/// @file userver/cache/ydb/cache.hpp
/// @brief @copybrief components::YdbCache

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
#include <userver/ydb/component.hpp>
#include <userver/ydb/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @page Caching Component for Ydb
///
/// A typical components::YdbCache usage consists of trait definition:
///
/// @snippet cache/ydb_cache_test.cpp ydb Cache Policy Trivial
///
/// and registration of the component in components::ComponentList:
///
/// @snippet cache/ydb_cache_test.cpp  ydb Cache Trivial Usage
///
/// See @ref scripts/docs/en/userver/caches.md for introduction into caches.
///
///
/// @section ydb_cc_configuration Configuration
///
/// components::YdbCache static configuration file should have a Ydb
/// component name specified in `ydbcomponent` configuration parameter.
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
/// chunk-size | number of rows to request from Ydb via portals, 0 to fetch all rows in one request without portals | 1000
///
/// @section ydb_cc_cache_policy Cache policy
///
/// Cache policy is the template argument of components::YdbCache component.
/// Please see the following code snippet for documentation.
///
/// @snippet cache/ydb_cache_test.cpp ydb Cache Policy Example
///
/// The query can be a std::string. But due to non-guaranteed order of static
/// data members initialization, std::string should be returned from a static
/// member function, please see the following code snippet.
///
/// @snippet cache/ydb_cache_test.cpp ydb Cache Policy GetQuery Example
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
/// @snippet cache/ydb_cache_test.cpp ydb Cache Policy Custom Updated Example
///
/// In case one provides a custom CacheContainer within Policy, it is notified
/// of Update completion via its public member function OnWritesDone, if any.
/// See the following code snippet for an example of usage:
///
/// @snippet cache/ydb_cache_test.cpp ydb Cache Policy Custom Container With Write Notification Example
///
/// @section ydb_cc_forward_declaration Forward Declaration
///
/// To forward declare a cache you can forward declare a trait and
/// include userver/cache/base_ydb_cache_fwd.hpp header. It is also useful to
/// forward declare the cache value type.
///
/// @snippet cache/ydb_cache_test_fwd.hpp ydb Cache Fwd Example
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref scripts/docs/en/userver/cache_dumps.md | @ref scripts/docs/en/userver/lru_cache.md ⇨
/// @htmlonly </div> @endhtmlonly

// clang-format on

namespace ydb_cache::detail {

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

template <typename YdbCachePolicy>
auto ExtractValue(RawValueType<YdbCachePolicy>&& raw) {
  if constexpr (kHasRawValueType<YdbCachePolicy>) {
    return Convert(std::move(raw),
                   formats::parse::To<ValueType<YdbCachePolicy>>());
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
    meta::DetectedOr<ydb::DateTime, UpdatedFieldTypeImpl, T>;

template <typename T>
constexpr bool CheckUpdatedFieldType() {
  if constexpr (kHasUpdatedFieldType<T>) {
    static_assert(
        std::is_same_v<typename T::UpdatedFieldType,
                       storages::ydb::DateTime> ||
            std::is_same_v<typename T::UpdatedFieldType,
                           storages::ydb::DateTime> ||
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
constexpr storages::ydb::ClusterHostType ClusterHostType() {
  if constexpr (meta::kIsDetected<HasClusterHostTypeImpl, T>) {
    return T::ClusterHostType;
  } else {
    return storages::ydb::ClusterHostType::kPrimary;
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

template <typename YdbCachePolicy>
struct PolicyChecker {
  // Static assertions for cache traits
  static_assert(
      kHasName<YdbCachePolicy>,
      "The PosgreSQL cache policy must contain a static member `kName`");
  static_assert(
      kHasValueType<YdbCachePolicy>,
      "The PosgreSQL cache policy must define a type alias `ValueType`");
  static_assert(
      kHasKeyMember<YdbCachePolicy>,
      "The Ydb cache policy must contain a static member `kKeyMember` "
      "with a pointer to a data or a function member with the object's key");
  static_assert(kHasQuery<YdbCachePolicy> ||
                    kHasGetQuery<YdbCachePolicy>,
                "The PosgreSQL cache policy must contain a static data member "
                "`kQuery` with a select statement or a static member function "
                "`GetQuery` returning the query");
  static_assert(!(kHasQuery<YdbCachePolicy> &&
                  kHasGetQuery<YdbCachePolicy>),
                "The PosgreSQL cache policy must define `kQuery` or "
                "`GetQuery`, not both");
  static_assert(
      kHasUpdatedField<YdbCachePolicy>,
      "The PosgreSQL cache policy must contain a static member "
      "`kUpdatedField`. If you don't want to use incremental updates, "
      "please set its value to `nullptr`");
  static_assert(CheckUpdatedFieldType<YdbCachePolicy>());

  /*
  static_assert(ClusterHostType<YdbCachePolicy>() &
                    storages::ydb::kClusterHostRolesMask,
                "Cluster host role must be specified for caching component, "
                "please be more specific");
  */

  static storages::ydb::Query GetQuery() {
    if constexpr (kHasGetQuery<YdbCachePolicy>) {
      return YdbCachePolicy::GetQuery();
    } else {
      return YdbCachePolicy::kQuery;
    }
  }

  using BaseType =
      CachingComponentBase<DataCacheContainerType<YdbCachePolicy>>;
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
}  // namespace ydb_cache::detail

/// @ingroup userver_components
///
/// @brief Caching component for Ydb. See @ref ydb_cache.
///
/// @see @ref ydb_cache, @ref scripts/docs/en/userver/caches.md
template <typename YdbCachePolicy>
class YdbCache final
    : public ydb_cache::detail::PolicyChecker<YdbCachePolicy>::BaseType {
 public:
  // Type aliases
  using PolicyType = YdbCachePolicy;
  using ValueType = ydb_cache::detail::ValueType<PolicyType>;
  using RawValueType = ydb_cache::detail::RawValueType<PolicyType>;
  using DataType = ydb_cache::detail::DataCacheContainerType<PolicyType>;
  using PolicyCheckerType = ydb_cache::detail::PolicyChecker<YdbCachePolicy>;
  using UpdatedFieldType =
      ydb_cache::detail::UpdatedFieldType<YdbCachePolicy>;
  using BaseType = typename PolicyCheckerType::BaseType;

  // Calculated constants
  constexpr static bool kIncrementalUpdates =
      ydb_cache::detail::kWantIncrementalUpdates<PolicyType>;
  constexpr static auto kClusterHostTypeFlags =
      ydb_cache::detail::ClusterHostType<PolicyType>();
  constexpr static auto kName = PolicyType::kName;

  YdbCache(const ComponentConfig&, const ComponentContext&);
  ~YdbCache() override;

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

  static storages::ydb::Query GetAllQuery();
  static storages::ydb::Query GetDeltaQuery();

  std::chrono::milliseconds ParseCorrection(const ComponentConfig& config);

  std::vector<std::shared_ptr<storages::ydb::Cluster>> clusters_;

  const std::chrono::system_clock::duration correction_;
  const std::chrono::milliseconds full_update_timeout_;
  const std::chrono::milliseconds incremental_update_timeout_;
  const std::size_t chunk_size_;
  std::size_t cpu_relax_iterations_parse_{0};
  std::size_t cpu_relax_iterations_copy_{0};
};

template <typename YdbCachePolicy>
inline constexpr bool kHasValidate<YdbCache<YdbCachePolicy>> = true;

template <typename YdbCachePolicy>
YdbCache<YdbCachePolicy>::YdbCache(const ComponentConfig& config,
                                               const ComponentContext& context)
    : BaseType{config, context},
      correction_{ParseCorrection(config)},
      full_update_timeout_{
          config["full-update-op-timeout"].As<std::chrono::milliseconds>(
              ydb_cache::detail::kDefaultFullUpdateTimeout)},
      incremental_update_timeout_{
          config["incremental-update-op-timeout"].As<std::chrono::milliseconds>(
              ydb_cache::detail::kDefaultIncrementalUpdateTimeout)},
      chunk_size_{config["chunk-size"].As<size_t>(
          ydb_cache::detail::kDefaultChunkSize)} {
  /* TODO
  UINVARIANT(
      !chunk_size_ || storages::ydb::Portal::IsSupportedByDriver(),
      "Either set 'chunk-size' to 0, or enable Ydb portals by building "
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

  const auto ydb_alias = config["ydbcomponent"].As<std::string>("");
  /* TODO
  if (ydb_alias.empty()) {
    throw storages::ydb::InvalidConfig{
        "No `ydbcomponent` entry in configuration"};
  }
  */
  auto& ydb_cluster_comp = context.FindComponent<userver::storages::ydb::Component>(ydb_alias);
  const auto shard_count = 1;
  clusters_.resize(shard_count);
  for (size_t i = 0; i < shard_count; ++i) {
    clusters_[i] = ydb_cluster_comp.GetCluster();
  }

  LOG_INFO() << "Cache " << kName << " full update query `"
             << GetAllQuery().GetStatement() << "` incremental update query `"
             << GetDeltaQuery().GetStatement() << "`";

  this->StartPeriodicUpdates();
}

template <typename YdbCachePolicy>
YdbCache<YdbCachePolicy>::~YdbCache() {
  this->StopPeriodicUpdates();
}

template <typename YdbCachePolicy>
storages::ydb::Query YdbCache<YdbCachePolicy>::GetAllQuery() {
  storages::ydb::Query query = PolicyCheckerType::GetQuery();
  if constexpr (ydb_cache::detail::kHasWhere<YdbCachePolicy>) {
    return {fmt::format("{} where {}", query.GetStatement(),
                        YdbCachePolicy::kWhere)};
  } else {
    return query;
  }
}

template <typename YdbCachePolicy>
storages::ydb::Query YdbCache<YdbCachePolicy>::GetDeltaQuery() {
  if constexpr (kIncrementalUpdates) {
    storages::ydb::Query query = PolicyCheckerType::GetQuery();
    if constexpr (ydb_cache::detail::kHasWhere<YdbCachePolicy>) {
      return {
          fmt::format("{} where ({}) and {} >= $1", query.GetStatement(),
                      YdbCachePolicy::kWhere, PolicyType::kUpdatedField)};
    } else {
      return {fmt::format("{} where {} >= $1", query.GetStatement(),
                          PolicyType::kUpdatedField)};
    }
  } else {
    return GetAllQuery();
  }
}

template <typename YdbCachePolicy>
std::chrono::milliseconds YdbCache<YdbCachePolicy>::ParseCorrection(
    const ComponentConfig& config) {
  static constexpr std::string_view kUpdateCorrection = "update-correction";
  if (ydb_cache::detail::kHasCustomUpdated<YdbCachePolicy> ||
      this->GetAllowedUpdateTypes() == cache::AllowedUpdateTypes::kOnlyFull) {
    return config[kUpdateCorrection].As<std::chrono::milliseconds>(0);
  } else {
    return config[kUpdateCorrection].As<std::chrono::milliseconds>();
  }
}

template <typename YdbCachePolicy>
typename YdbCache<YdbCachePolicy>::UpdatedFieldType
YdbCache<YdbCachePolicy>::GetLastUpdated(
    [[maybe_unused]] std::chrono::system_clock::time_point last_update,
    const DataType& cache) const {
  if constexpr (ydb_cache::detail::kHasCustomUpdated<YdbCachePolicy>) {
    return YdbCachePolicy::GetLastKnownUpdated(cache);
  } else {
    return UpdatedFieldType{last_update - correction_};
  }
}

template <typename YdbCachePolicy>
void YdbCache<YdbCachePolicy>::Update(
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
      std::string{ydb_cache::detail::kCopyStage});
  auto data_cache = GetDataSnapshot(type, scope);
  [[maybe_unused]] const auto old_size = data_cache->size();

  scope.Reset(std::string{ydb_cache::detail::kFetchStage});

  size_t changes = 0;
  // Iterate clusters
  // TODO
  for (const std::shared_ptr<storages::ydb::Cluster>& cluster : clusters_) {
    if (chunk_size_ > 0) {
      /*auto trx = cluster->Begin(kClusterHostTypeFlags);
      auto portal =
          trx.MakePortal(query, GetLastUpdated(last_update, *data_cache));
      while (portal) {
        scope.Reset(std::string{ydb_cache::detail::kFetchStage});
        auto res = portal.Fetch(chunk_size_);
        stats_scope.IncreaseDocumentsReadCount(res.Size());

        scope.Reset(std::string{ydb_cache::detail::kParseStage});
        CacheResults(res, data_cache, stats_scope, scope);
        changes += res.Size();
      }
      trx.Commit();*/
    } else {
      //bool has_parameter = query.GetStatement().find('$') != std::string::npos;
      auto resultValues = cluster->Execute(userver::storages::ydb::ClusterHostType::kPrimary, query.GetStatement()).AsVector<ValueType>();
      stats_scope.IncreaseDocumentsReadCount(resultValues.size());

      scope.Reset(std::string{ydb_cache::detail::kParseStage});
      CacheResults(resultValues, data_cache, stats_scope, scope);
      changes += resultValues.size();
    }
  }

  scope.Reset();

  if constexpr (ydb_cache::detail::kIsContainerCopiedByElement<DataType>) {
    if (old_size > 0) {
      const auto elapsed_copy =
          scope.ElapsedTotal(std::string{ydb_cache::detail::kCopyStage});
      if (elapsed_copy > ydb_cache::detail::kCpuRelaxThreshold) {
        cpu_relax_iterations_copy_ = static_cast<std::size_t>(
            static_cast<double>(old_size) /
            (elapsed_copy / ydb_cache::detail::kCpuRelaxInterval));
        LOG_TRACE() << "Elapsed time for copying " << kName << " "
                    << elapsed_copy.count() << " for " << changes
                    << " data items is over threshold. Will relax CPU every "
                    << cpu_relax_iterations_parse_ << " iterations";
      }
    }
  }

  if (changes > 0) {
    const auto elapsed_parse =
        scope.ElapsedTotal(std::string{ydb_cache::detail::kParseStage});
    if (elapsed_parse > ydb_cache::detail::kCpuRelaxThreshold) {
      cpu_relax_iterations_parse_ = static_cast<std::size_t>(
          static_cast<double>(changes) /
          (elapsed_parse / ydb_cache::detail::kCpuRelaxInterval));
      LOG_TRACE() << "Elapsed time for parsing " << kName << " "
                  << elapsed_parse.count() << " for " << changes
                  << " data items is over threshold. Will relax CPU every "
                  << cpu_relax_iterations_parse_ << " iterations";
    }
  }
  if (changes > 0 || type == cache::UpdateType::kFull) {
    // Set current cache
    stats_scope.Finish(data_cache->size());
    ydb_cache::detail::OnWritesDone(*data_cache);
    this->Set(std::move(data_cache));
  } else {
    stats_scope.FinishNoChanges();
  }
}

template <typename YdbCachePolicy>
bool YdbCache<YdbCachePolicy>::MayReturnNull() const {
  return ydb_cache::detail::MayReturnNull<PolicyType>();
}

template <typename YdbCachePolicy>
void YdbCache<YdbCachePolicy>::CacheResults(
    std::vector<ValueType> res, CachedData& data_cache,
    cache::UpdateStatisticsScope& stats_scope, tracing::ScopeTime& scope) {
  auto values = res;
  utils::CpuRelax relax{cpu_relax_iterations_parse_, &scope};
  for (auto p = values.begin(); p != values.end(); ++p) {
    relax.Relax();
    try {
      using ydb_cache::detail::CacheInsertOrAssign;
      CacheInsertOrAssign(
          *data_cache, *p,
          YdbCachePolicy::kKeyMember);
    } catch (const std::exception& e) {
      stats_scope.IncreaseDocumentsParseFailures(1);
      LOG_ERROR() << "Error parsing data row in cache '" << kName << "' to '"
                  << compiler::GetTypeName<ValueType>() << "': " << e.what();
    }
  }
}

template <typename YdbCachePolicy>
typename YdbCache<YdbCachePolicy>::CachedData
YdbCache<YdbCachePolicy>::GetDataSnapshot(cache::UpdateType type,
                                                  tracing::ScopeTime& scope) {
  if (type == cache::UpdateType::kIncremental) {
    auto data = this->Get();
    if (data) {
      return ydb_cache::detail::CopyContainer(*data, cpu_relax_iterations_copy_,
                                             scope);
    }
  }
  return std::make_unique<DataType>();
}

namespace impl {

}  // namespace impl

template <typename YdbCachePolicy>
yaml_config::Schema YdbCache<YdbCachePolicy>::GetStaticConfigSchema() {
  using ParentType =
      typename ydb_cache::detail::PolicyChecker<YdbCachePolicy>::BaseType;
  const static std::string schema = R"(
type: object
description: Caching component for Ydb derived from components::CachingComponentBase.
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
        description: number of rows to request from Ydb, 0 to fetch all rows in one request
        defaultDescription: 1000
    ydbcomponent:
        type: string
        description: Ydb component name
        defaultDescription: ""
)";
  return yaml_config::MergeSchemas<ParentType>(schema);
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