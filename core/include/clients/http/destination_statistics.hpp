#pragma once

#include <memory>
#include <unordered_map>

#include <rcu/rcu.hpp>

#include <clients/http/statistics.hpp>

namespace clients {
namespace http {

class DestinationStatistics {
 public:
  // Return pointer to related RequestStats
  std::shared_ptr<RequestStats> GetStatisticsForDestination(
      const std::string& destination);

  // If max_auto_destinations reached, return nullptr, pointer to related
  // RequestStats otherwise
  std::shared_ptr<RequestStats> GetStatisticsForDestinationAuto(
      const std::string& destination);

  void SetAutoMaxSize(size_t max_auto_destinations);

  using DestinationsMap =
      std::unordered_map<std::string, std::shared_ptr<Statistics>>;

  DestinationsMap GetCurrentMap() const;

 private:
  std::shared_ptr<RequestStats> GetExistingStatisticsForDestination(
      const std::string& destination);

  std::shared_ptr<RequestStats> CreateStatisticsForDestination(
      const std::string& destination);

 private:
  rcu::Variable<DestinationsMap> rcu_map_;
  size_t max_auto_destinations_{0};
  std::atomic<size_t> current_auto_destinations_{{0}};
};

}  // namespace http
}  // namespace clients
