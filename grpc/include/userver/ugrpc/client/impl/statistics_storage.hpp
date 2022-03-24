#pragma once

#include <string_view>
#include <unordered_map>

#include <userver/engine/shared_mutex.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// Clients are created on-the-fly, so we must use a separate stable container
/// for storing their statistics.
class StatisticsStorage final {
 public:
  explicit StatisticsStorage(utils::statistics::Storage& statistics_storage);

  ~StatisticsStorage();

  ugrpc::impl::ServiceStatistics& GetServiceStatistics(
      const ugrpc::impl::StaticServiceMetadata& metadata);

 private:
  // Pointer to service name from its metadata is used as a unique service ID
  using ServiceId = const char*;

  formats::json::Value ExtendStatistics(std::string_view prefix);

  std::unordered_map<ServiceId, ugrpc::impl::ServiceStatistics>
      service_statistics_;
  engine::SharedMutex mutex_;

  utils::statistics::Entry statistics_holder_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
