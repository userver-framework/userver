#pragma once

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
      const ugrpc::impl::StaticServiceMetadata& metadata);

  // Can only be called on StatisticsStorage for gRPC services (not clients).
  // Can only be called strictly after all the components are loaded.
  // gRPC services must not be [un]registered during GetStartedRequests().
  std::uint64_t GetStartedRequests() const;

 private:
  // Pointer to service name from its metadata is used as a unique service ID
  using ServiceId = const char*;

  void ExtendStatistics(utils::statistics::Writer& writer);

  // std::equal_to<const char*> specialized in arcadia/util/str_stl.h
  // so we need to define our own comparer to use it in map and avoid
  // specialization after instantiation problem
  struct ServiceIdComparer final {
    bool operator()(ServiceId lhs, ServiceId rhs) const { return lhs == rhs; }
  };

  const StatisticsDomain domain_;

  std::unordered_map<ServiceId, ugrpc::impl::ServiceStatistics,
                     std::hash<ServiceId>, ServiceIdComparer>
      service_statistics_;
  engine::SharedMutex mutex_;

  utils::statistics::Entry statistics_holder_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
