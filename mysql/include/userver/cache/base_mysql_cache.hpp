#pragma once

#include <fmt/format.h>

#include <userver/tracing/span.hpp>

#include <userver/cache/mysql_cache_type_traits.hpp>
#include <userver/storages/mysql/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

template <typename MySQLCachePolicy>
class MySQLCache final
    : public mysql_cache::impl::PolicyChecker<MySQLCachePolicy>::BaseType {
 public:
  // Type aliases
  using PolicyType = MySQLCachePolicy;
  using ValueType = mysql_cache::impl::ValueType<PolicyType>;
  using RawValueType = mysql_cache::impl::RawValueType<PolicyType>;
  using DataType = mysql_cache::impl::DataCacheContainerType<PolicyType>;
  using PolicyCheckerType = mysql_cache::impl::PolicyChecker<PolicyType>;
  using UpdatedFieldType = mysql_cache::impl::UpdatedFieldType<PolicyType>;
  using BaseType = typename PolicyCheckerType::BaseType;

  // calculated constants
  static constexpr bool kIncrementalUpdates =
      mysql_cache::impl::kWantsIncrementalUpdates<PolicyType>;
  static constexpr auto kClusterHostTypeFlags =
      mysql_cache::impl::ClusterHostType<PolicyType>();
  constexpr static auto kName = PolicyType::kName;

  MySQLCache(const ComponentConfig& config, const ComponentContext& context);
  ~MySQLCache() override;

 private:
  using CachedData = std::unique_ptr<DataType>;

  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope& stats_scope) final;

  CachedData GetDataSnapshot(cache::UpdateType type, tracing::ScopeTime& scope);

  static std::string GetAllQuery();
  static std::string GetDeltaQuery();

  std::shared_ptr<storages::mysql::Cluster> cluster_{nullptr};
};

template <typename MySQLCachePolicy>
MySQLCache<MySQLCachePolicy>::MySQLCache(const ComponentConfig& config,
                                         const ComponentContext& context)
    : BaseType{config, context} {
  this->StartPeriodicUpdates();
}

template <typename MySQLCachePolicy>
MySQLCache<MySQLCachePolicy>::~MySQLCache() {
  this->StopPeriodicUpdates();
}

template <typename MySQLCachePolicy>
std::string MySQLCache<MySQLCachePolicy>::GetAllQuery() {
  const auto query = PolicyCheckerType::GetQuery();
  if constexpr (mysql_cache::impl::kHasWhere<MySQLCachePolicy>) {
    return fmt::format("{} WHERE {}", query, MySQLCachePolicy::kWhere);
  } else {
    return query;
  }
}

template <typename MySQLCachePolicy>
std::string MySQLCache<MySQLCachePolicy>::GetDeltaQuery() {
  if constexpr (kIncrementalUpdates) {
    const auto query = PolicyCheckerType::GetQuery();

    if constexpr (mysql_cache::impl::kHasWhere<MySQLCachePolicy>) {
      return fmt::format("{} WHERE ({}) and {} >= ?", query,
                         MySQLCachePolicy::kWhere, PolicyType::kUpdatedField);
    } else {
      return fmt::format("{} WHERE {} >= ?", query, PolicyType::kUpdatedField);
    }
  } else {
    return GetAllQuery();
  }
}

template <typename MySQLCachePolicy>
void MySQLCache<MySQLCachePolicy>::Update(
    cache::UpdateType type,
    const std::chrono::system_clock::time_point& /* last_update */,
    const std::chrono::system_clock::time_point& /* now */,
    cache::UpdateStatisticsScope& stats_scope) {
  if constexpr (!kIncrementalUpdates) {
    type = cache::UpdateType::kFull;
  }

  const auto query =
      (type == cache::UpdateType::kFull) ? GetAllQuery() : GetDeltaQuery();
  // TODO
  const std::chrono::milliseconds timeout{1000};

  // copy current cached data
  auto scope = tracing::Span::CurrentSpan().CreateScopeTime("COPY_TODO");
  auto data_cache = GetDataSnapshot(type, scope);

  const auto deadline = engine::Deadline::FromDuration(timeout);
  std::size_t changes{0};
  cluster_
      ->GetCursor<RawValueType>(kClusterHostTypeFlags,
                                // TODO : batch size
                                deadline, 10, query)
      .ForEach([](auto&&) {}, deadline);

  scope.Reset();

  if (changes > 0 || type == cache::UpdateType::kFull) {
    stats_scope.Finish(data_cache->size());
    mysql_cache::impl::OnWritesDone(*data_cache);
    this->Set(std::move(data_cache));
  } else {
    stats_scope.FinishNoChanges();
  }
}

template <typename MySQLCachePolicy>
typename MySQLCache<MySQLCachePolicy>::CachedData
MySQLCache<MySQLCachePolicy>::GetDataSnapshot(cache::UpdateType type,
                                              tracing::ScopeTime& scope) {
  if (type == cache::UpdateType::kIncremental) {
    auto data = this->Get();
    if (data) {
      // TODO : fix 100
      return mysql_cache::impl::CopyContainer(*data, 100, scope);
    }
  }
  return std::make_unique<DataType>();
}

}  // namespace components

USERVER_NAMESPACE_END
