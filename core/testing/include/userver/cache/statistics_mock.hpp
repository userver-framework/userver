#pragma once

/// @file userver/cache/statistics_mock.hpp
/// @brief @copybrief cache::UpdateStatisticsScopeMock

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/update_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// @brief Allows to test helper functions of CacheUpdateTrait::Update that use
/// UpdateStatisticsScope
class UpdateStatisticsScopeMock final {
 public:
  explicit UpdateStatisticsScopeMock(UpdateType type);

  UpdateStatisticsScope& GetScope();

 private:
  impl::Statistics stats_;
  UpdateStatisticsScope scope_;
};

}  // namespace cache

USERVER_NAMESPACE_END
