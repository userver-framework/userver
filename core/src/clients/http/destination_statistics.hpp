#pragma once

#include <memory>
#include <unordered_map>

#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <clients/http/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class DestinationStatistics final {
 public:
  // Return pointer to related RequestStats
  std::shared_ptr<RequestStats> GetStatisticsForDestination(
      const std::string& destination);

  // If max_auto_destinations reached, return nullptr, pointer to related
  // RequestStats otherwise
  std::shared_ptr<RequestStats> GetStatisticsForDestinationAuto(
      const std::string& destination);

  void SetAutoMaxSize(size_t max_auto_destinations);

  using DestinationsMap = rcu::RcuMap<std::string, Statistics>;

  DestinationsMap::ConstIterator begin() const;
  DestinationsMap::ConstIterator end() const;

 private:
  std::shared_ptr<RequestStats> GetExistingStatisticsForDestination(
      const std::string& destination);

  std::shared_ptr<RequestStats> CreateStatisticsForDestination(
      const std::string& destination);

  rcu::RcuMap<std::string, Statistics> rcu_map_;
  size_t max_auto_destinations_{0};
  std::atomic<size_t> current_auto_destinations_{0};
};

void DumpMetric(utils::statistics::Writer& writer,
                const DestinationStatistics& stats);

}  // namespace clients::http

USERVER_NAMESPACE_END
