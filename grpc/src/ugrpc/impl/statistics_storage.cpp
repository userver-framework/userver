#include <userver/ugrpc/impl/statistics_storage.hpp>

#include <fmt/format.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

StatisticsStorage::StatisticsStorage(
    utils::statistics::Storage& statistics_storage, std::string_view domain) {
  statistics_holder_ = statistics_storage.RegisterWriter(
      fmt::format("grpc.{}", domain),
      [this](utils::statistics::Writer& writer) { ExtendStatistics(writer); });
}

StatisticsStorage::~StatisticsStorage() { statistics_holder_.Unregister(); }

ugrpc::impl::ServiceStatistics& StatisticsStorage::GetServiceStatistics(
    const ugrpc::impl::StaticServiceMetadata& metadata) {
  // We exploit the fact that 'service_full_name' always points to the same
  // static string for a given service.
  const ServiceId service_id = metadata.service_full_name.data();

  {
    std::shared_lock lock(mutex_);
    if (auto* stats = utils::FindOrNullptr(service_statistics_, service_id)) {
      return *stats;
    }
  }

  // All the other clients are blocked while we instantiate stats for a new
  // service. This is OK, because it will only happen a finite number of times
  // during startup.
  std::lock_guard lock(mutex_);

  const auto [iter, is_new] =
      service_statistics_.try_emplace(service_id, metadata);
  return iter->second;
}

void StatisticsStorage::ExtendStatistics(utils::statistics::Writer& writer) {
  std::shared_lock lock(mutex_);
  {
    auto by_destination = writer["by-destination"];
    for (const auto& [_, service_stats] : service_statistics_) {
      by_destination = service_stats;
    }
  }
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
