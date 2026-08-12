#pragma once

/// @file userver/cache/base_postgres_cache.hpp
/// @brief @copybrief components::PostgreCache

#include <userver/cache/base_postgres_cache_fwd.hpp>

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

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/utils/meta.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace pg_cache::detail {

template <typename T>
using ValueType = typename T::ValueType;
template <typename T>
concept HasValueType = requires { typename T::ValueType; };

template <typename T>
struct RawValueTypeImpl : std::type_identity<ValueType<T>> {};

template <typename T>
concept HasRawValueType = requires { typename T::RawValueType; };

template <HasRawValueType T>
struct RawValueTypeImpl<T> : std::type_identity<typename T::RawValueType> {};

template <typename T>
using RawValueType = typename RawValueTypeImpl<T>::type;

template <typename PostgreCachePolicy>
auto ExtractValue(RawValueType<PostgreCachePolicy>&& raw) {
    if constexpr (HasRawValueType<PostgreCachePolicy>) {
        return Convert(std::move(raw), formats::parse::To<ValueType<PostgreCachePolicy>>());
    } else {
        return std::move(raw);
    }
}

// Component name in policy
template <typename T>
concept HasName = requires {
    requires !std::string_view { T::kName }
    .empty();
};

// Component query in policy
template <typename T>
concept HasQuery = requires { T::kQuery; };

// Component GetQuery in policy
template <typename T>
concept HasGetQuery = requires { T::GetQuery(); };

// Component kWhere in policy
template <typename T>
concept HasWhere = requires { T::kWhere; };

// Component kOrderBy in policy
template <typename T>
concept HasOrderBy = requires { T::kOrderBy; };

// Component kTag in policy
template <typename T>
concept HasTag = requires { T::kTag; };

template <typename PostgreCachePolicy>
auto GetTag() {
    if constexpr (HasTag<PostgreCachePolicy>) {
        return PostgreCachePolicy::kTag;
    } else {
        return storages::postgres::kRowTag;
    }
}

// Update field
template <typename T>
concept HasUpdatedField = requires { T::kUpdatedField; };

template <typename T>
concept WantIncrementalUpdates = requires {
    {
        std::integral_constant<bool, !std::string_view{T::kUpdatedField}.empty()>{}
    } -> std::same_as<std::true_type>;
};

// Key member in policy
template <typename T>
concept HasKeyMember = requires { T::kKeyMember; } && std::invocable<decltype(T::kKeyMember), ValueType<T>>;

template <typename T>
using KeyMemberType = std::decay_t<std::invoke_result_t<decltype(T::kKeyMember), ValueType<T>>>;

// size method in custom container in policy
template <typename T>
concept HasSizeMethod = requires(T t) {
    {
        t.size()
    } -> std::convertible_to<std::size_t>;
};

// insert_or_assign method in custom container in policy
template <typename T>
concept HasInsertOrAssignMethod = requires(typename T::CacheContainer& c, KeyMemberType<T> key, ValueType<T> val) {
    c.insert_or_assign(std::move(key), std::move(val));
};

// CacheInsertOrAssign function for custom container in policy
template <typename T>
concept HasCacheInsertOrAssignFunction =
    requires(typename T::CacheContainer& c, ValueType<T> val, KeyMemberType<T> key) {
        CacheInsertOrAssign(c, std::move(val), std::move(key));
    };

template <typename T>
concept PolicyHasCustomCacheContainer = requires { typename T::CacheContainer; };

// Data container for cache
template <typename T>
struct DataCacheContainer {
    static_assert(
        meta::IsStdHashable<KeyMemberType<T>>,
        "With default CacheContainer, key type must be std::hash-able"
    );

    using type = std::unordered_map<KeyMemberType<T>, ValueType<T>>;
};

template <typename T>
requires PolicyHasCustomCacheContainer<T>
struct DataCacheContainer<T> {
    static_assert(HasSizeMethod<typename T::CacheContainer>, "Custom CacheContainer must provide `size` method");
    static_assert(
        HasInsertOrAssignMethod<T> || HasCacheInsertOrAssignFunction<T>,
        "Custom CacheContainer must provide `insert_or_assign`  method similar to std::unordered_map's "
        "one or CacheInsertOrAssign function"
    );

    using type = typename T::CacheContainer;
};

template <typename T>
using DataCacheContainerType = typename DataCacheContainer<T>::type;

// We have to whitelist container types, for which we perform by-element
// copying, because it's not correct for certain custom containers.
template <typename T>
inline constexpr bool kIsContainerCopiedByElement =
    meta::kIsInstantiationOf<std::unordered_map, T> || meta::kIsInstantiationOf<std::map, T>;

template <typename T>
std::unique_ptr<T> CopyContainer(
    const T& container,
    [[maybe_unused]] std::size_t cpu_relax_iterations,
    tracing::ScopeTime& scope
) {
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

template <typename Container, typename Value, typename KeyMember, typename... Args>
void CacheInsertOrAssign(Container& container, Value&& value, const KeyMember& key_member, Args&&... /*args*/) {
    // Args are only used to de-prioritize this default overload.
    static_assert(sizeof...(Args) == 0);
    // Copy 'key' to avoid aliasing issues in 'insert_or_assign'.
    auto key = std::invoke(key_member, value);
    container.insert_or_assign(std::move(key), std::forward<Value>(value));
}

template <typename T>
void OnWritesDone(T& container) {
    if constexpr (requires(T& c) { c.OnWritesDone(); }) {
        container.OnWritesDone();
    }
}

template <typename T>
concept HasCustomUpdated = requires(const DataCacheContainerType<T>& cache) { T::GetLastKnownUpdated(cache); };

template <typename T>
struct UpdatedFieldTypeImpl : std::type_identity<storages::postgres::TimePointTz> {};

template <typename T>
concept HasUpdatedFieldType = requires { typename T::UpdatedFieldType; };

template <HasUpdatedFieldType T>
struct UpdatedFieldTypeImpl<T> : std::type_identity<typename T::UpdatedFieldType> {};

template <typename T>
using UpdatedFieldType = typename UpdatedFieldTypeImpl<T>::type;

template <typename T>
constexpr bool CheckUpdatedFieldType() {
    if constexpr (HasUpdatedFieldType<T>) {
#if USERVER_POSTGRES_ENABLE_LEGACY_TIMESTAMP
        static_assert(
            std::is_same_v<typename T::UpdatedFieldType, storages::postgres::TimePointTz> ||
                std::is_same_v<typename T::UpdatedFieldType, storages::postgres::TimePointWithoutTz> ||
                std::is_same_v<typename T::UpdatedFieldType, storages::postgres::TimePoint> || HasCustomUpdated<T>,
            "Invalid UpdatedFieldType, must be either TimePointTz or "
            "TimePointWithoutTz"
            "or (legacy) system_clock::time_point"
        );
#else
        static_assert(
            std::is_same_v<typename T::UpdatedFieldType, storages::postgres::TimePointTz> ||
                std::is_same_v<typename T::UpdatedFieldType, storages::postgres::TimePointWithoutTz> ||
                HasCustomUpdated<T>,
            "Invalid UpdatedFieldType, must be either TimePointTz or "
            "TimePointWithoutTz"
        );
#endif
    } else {
        static_assert(
            !WantIncrementalUpdates<T>,
            "UpdatedFieldType must be explicitly specified when using "
            "incremental updates"
        );
    }
    return true;
}

// Cluster host type policy
template <typename T>
constexpr storages::postgres::ClusterHostTypeFlags ClusterHostType() {
    if constexpr (requires { T::kClusterHostType; }) {
        return T::kClusterHostType;
    } else {
        return storages::postgres::ClusterHostType::kSlave;
    }
}

// May return null policy
template <typename T>
constexpr bool MayReturnNull() {
    if constexpr (requires { T::kMayReturnNull; }) {
        return T::kMayReturnNull;
    } else {
        return false;
    }
}

template <typename PostgreCachePolicy>
struct PolicyChecker {
    // Static assertions for cache traits
    static_assert(HasName<PostgreCachePolicy>, "The PosgreSQL cache policy must contain a static member `kName`");
    static_assert(HasValueType<PostgreCachePolicy>, "The PosgreSQL cache policy must define a type alias `ValueType`");
    static_assert(
        HasKeyMember<PostgreCachePolicy>,
        "The PostgreSQL cache policy must contain a static member `kKeyMember` "
        "with a pointer to a data or a function member with the object's key"
    );
    static_assert(
        HasQuery<PostgreCachePolicy> || HasGetQuery<PostgreCachePolicy>,
        "The PosgreSQL cache policy must contain a static data member "
        "`kQuery` with a select statement or a static member function "
        "`GetQuery` returning the query"
    );
    static_assert(
        !(HasQuery<PostgreCachePolicy> && HasGetQuery<PostgreCachePolicy>),
        "The PosgreSQL cache policy must define `kQuery` or "
        "`GetQuery`, not both"
    );
    static_assert(
        HasUpdatedField<PostgreCachePolicy>,
        "The PosgreSQL cache policy must contain a static member "
        "`kUpdatedField`. If you don't want to use incremental updates, "
        "please set its value to `nullptr`"
    );
    static_assert(CheckUpdatedFieldType<PostgreCachePolicy>());

    static_assert(
        ClusterHostType<PostgreCachePolicy>() & storages::postgres::kClusterHostRolesMask,
        "Cluster host role must be specified for caching component, "
        "please be more specific"
    );

    static storages::postgres::Query GetQuery() {
        if constexpr (HasGetQuery<PostgreCachePolicy>) {
            return PostgreCachePolicy::GetQuery();
        } else {
            return PostgreCachePolicy::kQuery;
        }
    }

    using BaseType = CachingComponentBase<DataCacheContainerType<PostgreCachePolicy>>;
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
inline constexpr std::chrono::milliseconds kDefaultSleepBetweenChunks{0};
}  // namespace pg_cache::detail

/// @ingroup userver_components
///
/// @brief Caching component for PostgreSQL. See @ref scripts/docs/en/userver/pg/cache.md.
///
/// @see @ref scripts/docs/en/userver/pg/cache.md, @ref scripts/docs/en/userver/caches.md
template <typename PostgreCachePolicy>
class PostgreCache final : public pg_cache::detail::PolicyChecker<PostgreCachePolicy>::BaseType {
public:
    // Type aliases
    using PolicyType = PostgreCachePolicy;
    using ValueType = pg_cache::detail::ValueType<PolicyType>;
    using RawValueType = pg_cache::detail::RawValueType<PolicyType>;
    using DataType = pg_cache::detail::DataCacheContainerType<PolicyType>;
    using PolicyCheckerType = pg_cache::detail::PolicyChecker<PostgreCachePolicy>;
    using UpdatedFieldType = pg_cache::detail::UpdatedFieldType<PostgreCachePolicy>;
    using BaseType = typename PolicyCheckerType::BaseType;

    // Calculated constants
    constexpr static bool kIncrementalUpdates = pg_cache::detail::WantIncrementalUpdates<PolicyType>;
    constexpr static auto kClusterHostTypeFlags = pg_cache::detail::ClusterHostType<PolicyType>();
    constexpr static auto kName = PolicyType::kName;

    PostgreCache(const ComponentConfig&, const ComponentContext&);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    using CachedData = std::unique_ptr<DataType>;

    UpdatedFieldType GetLastUpdated(std::chrono::system_clock::time_point last_update, const DataType& cache) const;

    void Update(
        cache::UpdateType type,
        const std::chrono::system_clock::time_point& last_update,
        const std::chrono::system_clock::time_point& now,
        cache::UpdateStatisticsScope& stats_scope
    ) override;

    bool MayReturnNull() const override;

    CachedData GetDataSnapshot(cache::UpdateType type, tracing::ScopeTime& scope);
    void CacheResults(
        storages::postgres::ResultSet res,
        CachedData& data_cache,
        cache::UpdateStatisticsScope& stats_scope,
        tracing::ScopeTime& scope
    );

    static storages::postgres::Query GetAllQuery();
    static storages::postgres::Query GetDeltaQuery();
    static std::string GetWhereClause();
    static std::string GetDeltaWhereClause();
    static std::string GetOrderByClause();

    std::chrono::milliseconds ParseCorrection(const ComponentConfig& config);

    std::vector<storages::postgres::ClusterPtr> clusters_;

    const std::chrono::system_clock::duration correction_;
    const std::chrono::milliseconds full_update_timeout_;
    const std::chrono::milliseconds incremental_update_timeout_;
    const std::size_t chunk_size_;
    const std::chrono::milliseconds sleep_between_chunks_;
    std::size_t cpu_relax_iterations_parse_{0};
    std::size_t cpu_relax_iterations_copy_{0};
};

template <typename PostgreCachePolicy>
inline constexpr bool kHasValidate<PostgreCache<PostgreCachePolicy>> = true;

template <typename PostgreCachePolicy>
PostgreCache<PostgreCachePolicy>::PostgreCache(const ComponentConfig& config, const ComponentContext& context)
    : BaseType{config, context},
      correction_{ParseCorrection(config)},
      full_update_timeout_{
          config["full-update-op-timeout"].As<std::chrono::milliseconds>(pg_cache::detail::kDefaultFullUpdateTimeout)
      },
      incremental_update_timeout_{
          config["incremental-update-op-timeout"]
              .As<std::chrono::milliseconds>(pg_cache::detail::kDefaultIncrementalUpdateTimeout)
      },
      chunk_size_{config["chunk-size"].As<size_t>(pg_cache::detail::kDefaultChunkSize)},
      sleep_between_chunks_{
          config["sleep-between-chunks"].As<std::chrono::milliseconds>(pg_cache::detail::kDefaultSleepBetweenChunks)
      }
{
    UINVARIANT(
        !chunk_size_ || storages::postgres::Portal::IsSupportedByDriver(),
        "Either set 'chunk-size' to 0, or enable PostgreSQL portals by building "
        "the framework with CMake option USERVER_FEATURE_PATCH_LIBPQ set to ON."
    );

    if (this->GetAllowedUpdateTypes() == cache::AllowedUpdateTypes::kFullAndIncremental && !kIncrementalUpdates) {
        throw std::logic_error(
            "Incremental update support is requested in config but no update field "
            "name is specified in traits of '" +
            config.Name() + "' cache"
        );
    }
    if (correction_.count() < 0) {
        throw std::logic_error(
            "Refusing to set forward (negative) update correction requested in "
            "config for '" +
            config.Name() + "' cache"
        );
    }

    const auto pg_alias = config["pgcomponent"].As<std::string>("");
    if (pg_alias.empty()) {
        throw storages::postgres::InvalidConfig{"No `pgcomponent` entry in configuration"};
    }
    auto& pg_cluster_comp = context.FindComponent<components::Postgres>(pg_alias);
    const auto shard_count = pg_cluster_comp.GetShardCount();
    clusters_.resize(shard_count);
    for (size_t i = 0; i < shard_count; ++i) {
        clusters_[i] = pg_cluster_comp.GetClusterForShard(i);
    }

    LOG_INFO()
        << "Cache " << kName << " full update query `" << GetAllQuery().GetStatementView()
        << "` incremental update query `" << GetDeltaQuery().GetStatementView() << "`";
}

template <typename PostgreCachePolicy>
std::string PostgreCache<PostgreCachePolicy>::GetWhereClause() {
    if constexpr (pg_cache::detail::HasWhere<PostgreCachePolicy>) {
        return fmt::format(FMT_COMPILE("where {}"), PostgreCachePolicy::kWhere);
    } else {
        return "";
    }
}

template <typename PostgreCachePolicy>
std::string PostgreCache<PostgreCachePolicy>::GetDeltaWhereClause() {
    if constexpr (pg_cache::detail::HasWhere<PostgreCachePolicy>) {
        return fmt::format(
            FMT_COMPILE("where ({}) and {} >= $1"),
            PostgreCachePolicy::kWhere,
            PostgreCachePolicy::kUpdatedField
        );
    } else {
        return fmt::format(FMT_COMPILE("where {} >= $1"), PostgreCachePolicy::kUpdatedField);
    }
}

template <typename PostgreCachePolicy>
std::string PostgreCache<PostgreCachePolicy>::GetOrderByClause() {
    if constexpr (pg_cache::detail::HasOrderBy<PostgreCachePolicy>) {
        return fmt::format(FMT_COMPILE("order by {}"), PostgreCachePolicy::kOrderBy);
    } else {
        return "";
    }
}

template <typename PostgreCachePolicy>
storages::postgres::Query PostgreCache<PostgreCachePolicy>::GetAllQuery() {
    const storages::postgres::Query query = PolicyCheckerType::GetQuery();
    return fmt::format("{} {} {}", query.GetStatementView(), GetWhereClause(), GetOrderByClause());
}

template <typename PostgreCachePolicy>
storages::postgres::Query PostgreCache<PostgreCachePolicy>::GetDeltaQuery() {
    if constexpr (kIncrementalUpdates) {
        const storages::postgres::Query query = PolicyCheckerType::GetQuery();
        return storages::postgres::Query{
            fmt::format("{} {} {}", query.GetStatementView(), GetDeltaWhereClause(), GetOrderByClause()),
            query.GetOptionalName(),
        };
    } else {
        return GetAllQuery();
    }
}

template <typename PostgreCachePolicy>
std::chrono::milliseconds PostgreCache<PostgreCachePolicy>::ParseCorrection(const ComponentConfig& config) {
    static constexpr std::string_view kUpdateCorrection = "update-correction";
    if (pg_cache::detail::HasCustomUpdated<PostgreCachePolicy> ||
        this->GetAllowedUpdateTypes() == cache::AllowedUpdateTypes::kOnlyFull)
    {
        return config[kUpdateCorrection].As<std::chrono::milliseconds>(0);
    } else {
        return config[kUpdateCorrection].As<std::chrono::milliseconds>();
    }
}

template <typename PostgreCachePolicy>
typename PostgreCache<PostgreCachePolicy>::UpdatedFieldType PostgreCache<PostgreCachePolicy>::GetLastUpdated(
    [[maybe_unused]] std::chrono::system_clock::time_point last_update,
    const DataType& cache
) const {
    if constexpr (pg_cache::detail::HasCustomUpdated<PostgreCachePolicy>) {
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
    cache::UpdateStatisticsScope& stats_scope
) {
    namespace pg = storages::postgres;
    if constexpr (!kIncrementalUpdates) {
        type = cache::UpdateType::kFull;
    }
    const auto query = (type == cache::UpdateType::kFull) ? GetAllQuery() : GetDeltaQuery();
    const std::chrono::milliseconds
        timeout = (type == cache::UpdateType::kFull) ? full_update_timeout_ : incremental_update_timeout_;

    // COPY current cached data
    auto scope = tracing::Span::CurrentSpan().CreateScopeTime(std::string{pg_cache::detail::kCopyStage});
    auto data_cache = GetDataSnapshot(type, scope);
    [[maybe_unused]] const auto old_size = data_cache->size();

    scope.Reset(std::string{pg_cache::detail::kFetchStage});

    size_t changes = 0;
    // Iterate clusters
    for (auto& cluster : clusters_) {
        if (chunk_size_ > 0) {
            auto trx = cluster->Begin(
                kClusterHostTypeFlags,
                pg::Transaction::RO,
                pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff}
            );
            auto portal = trx.MakePortal(query, GetLastUpdated(last_update, *data_cache));
            while (portal) {
                scope.Reset(std::string{pg_cache::detail::kFetchStage});
                auto res = portal.Fetch(chunk_size_);
                stats_scope.IncreaseDocumentsReadCount(res.Size());

                scope.Reset(std::string{pg_cache::detail::kParseStage});
                CacheResults(res, data_cache, stats_scope, scope);
                changes += res.Size();
                if (sleep_between_chunks_.count() > 0) {
                    engine::InterruptibleSleepFor(sleep_between_chunks_);
                }
            }
            trx.Commit();
        } else {
            const bool has_parameter = query.GetStatementView().find('$') != std::string::npos;
            auto res =
                has_parameter
                    ? cluster->Execute(
                          kClusterHostTypeFlags,
                          pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff},
                          query,
                          GetLastUpdated(last_update, *data_cache)
                      )
                    : cluster->Execute(
                          kClusterHostTypeFlags,
                          pg::CommandControl{timeout, pg_cache::detail::kStatementTimeoutOff},
                          query
                      );
            stats_scope.IncreaseDocumentsReadCount(res.Size());

            scope.Reset(std::string{pg_cache::detail::kParseStage});
            CacheResults(res, data_cache, stats_scope, scope);
            changes += res.Size();
        }
    }

    scope.Reset();

    if constexpr (pg_cache::detail::kIsContainerCopiedByElement<DataType>) {
        if (old_size > 0) {
            const auto elapsed_copy = scope.ElapsedTotal(std::string{pg_cache::detail::kCopyStage});
            if (elapsed_copy > pg_cache::detail::kCpuRelaxThreshold) {
                cpu_relax_iterations_copy_ = static_cast<
                    std::size_t>(static_cast<double>(old_size) / (elapsed_copy / pg_cache::detail::kCpuRelaxInterval));
                LOG_TRACE()
                    << "Elapsed time for copying " << kName << " " << elapsed_copy.count() << " for " << changes
                    << " data items is over threshold. Will relax CPU every " << cpu_relax_iterations_parse_
                    << " iterations";
            }
        }
    }

    if (changes > 0) {
        const auto elapsed_parse = scope.ElapsedTotal(std::string{pg_cache::detail::kParseStage});
        if (elapsed_parse > pg_cache::detail::kCpuRelaxThreshold) {
            cpu_relax_iterations_parse_ = static_cast<
                std::size_t>(static_cast<double>(changes) / (elapsed_parse / pg_cache::detail::kCpuRelaxInterval));
            LOG_TRACE()
                << "Elapsed time for parsing " << kName << " " << elapsed_parse.count() << " for " << changes
                << " data items is over threshold. Will relax CPU every " << cpu_relax_iterations_parse_
                << " iterations";
        }
    }
    if (changes > 0 || type == cache::UpdateType::kFull) {
        // Set current cache
        pg_cache::detail::OnWritesDone(*data_cache);
        stats_scope.Finish(data_cache->size());
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
    storages::postgres::ResultSet res,
    CachedData& data_cache,
    cache::UpdateStatisticsScope& stats_scope,
    tracing::ScopeTime& scope
) {
    auto values = res.AsSetOf<RawValueType>(pg_cache::detail::GetTag<PostgreCachePolicy>());
    utils::CpuRelax relax{cpu_relax_iterations_parse_, &scope};
    for (auto p = values.begin(); p != values.end(); ++p) {
        relax.Relax();
        try {
            using pg_cache::detail::CacheInsertOrAssign;
            CacheInsertOrAssign(
                *data_cache,
                pg_cache::detail::ExtractValue<PostgreCachePolicy>(*p),
                PostgreCachePolicy::kKeyMember
            );
        } catch (const std::exception& e) {
            stats_scope.IncreaseDocumentsParseFailures(1);
            LOG_ERROR()
                << "Error parsing data row in cache '" << kName << "' to '" << compiler::GetTypeName<ValueType>()
                << "': " << e.what();
        }
    }
}

template <typename PostgreCachePolicy>
typename PostgreCache<PostgreCachePolicy>::CachedData PostgreCache<
    PostgreCachePolicy>::GetDataSnapshot(cache::UpdateType type, tracing::ScopeTime& scope) {
    if (type == cache::UpdateType::kIncremental) {
        auto data = this->Get();
        if (data) {
            return pg_cache::detail::CopyContainer(*data, cpu_relax_iterations_copy_, scope);
        }
    }
    return std::make_unique<DataType>();
}

namespace impl {

std::string GetPostgreCacheSchema();

}  // namespace impl

template <typename PostgreCachePolicy>
yaml_config::Schema PostgreCache<PostgreCachePolicy>::GetStaticConfigSchema() {
    using ParentType = typename pg_cache::detail::PolicyChecker<PostgreCachePolicy>::BaseType;
    return yaml_config::MergeSchemas<ParentType>(impl::GetPostgreCacheSchema());
}

}  // namespace components

namespace utils::impl::projected_set {

template <typename Set, typename Value, typename KeyMember>
void CacheInsertOrAssign(Set& set, Value&& value, const KeyMember& /*key_member*/) {
    DoInsert(set, std::forward<Value>(value));
}

}  // namespace utils::impl::projected_set

USERVER_NAMESPACE_END
