#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Clients are created on-the-fly, so we must use a separate stable container
/// for storing their statistics.
class StatisticsStorage final {
 public:
  explicit StatisticsStorage(utils::statistics::Storage& statistics_storage,
                             StatisticsDomain domain);

  StatisticsStorage(const StatisticsStorage&) = delete;
  StatisticsStorage& operator=(const StatisticsStorage&) = delete;

  ~StatisticsStorage();

  ugrpc::impl::ServiceStatistics& GetServiceStatistics(
      const ugrpc::impl::StaticServiceMetadata& metadata,
      std::optional<std::string> endpoint);

  // Can only be called on StatisticsStorage for gRPC services (not clients).
  // Can only be called strictly after all the components are loaded.
  // gRPC services must not be [un]registered during GetStartedRequests().
  std::uint64_t GetStartedRequests() const;

 private:
  // Pointer to service name from its metadata is used as a unique service ID
  using ServiceId = const char*;

  struct ServiceKey {
    ServiceId service_id;
    std::optional<std::string> endpoint;
  };

  void ExtendStatistics(utils::statistics::Writer& writer);

  struct ServiceKeyComparer final {
    bool operator()(ServiceKey lhs, ServiceKey rhs) const;
  };

  struct ServiceKeyHasher final {
    std::size_t operator()(const ServiceKey& key) const;
  };

  const StatisticsDomain domain_;

  std::unordered_map<ServiceKey, ugrpc::impl::ServiceStatistics,
                     ServiceKeyHasher, ServiceKeyComparer>
      service_statistics_;
  engine::SharedMutex mutex_;

  utils::statistics::Entry statistics_holder_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
