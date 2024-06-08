#include <userver/ugrpc/impl/statistics_storage.hpp>

#include <fmt/format.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

std::string_view ToString(StatisticsDomain domain) {
  static constexpr utils::TrivialBiMap kMap = [](auto selector) {
    return selector()
        .Case(StatisticsDomain::kClient, "client")
        .Case(StatisticsDomain::kServer, "server");
  };
  return utils::impl::EnumToStringView(domain, kMap);
}

StatisticsStorage::StatisticsStorage(
    utils::statistics::Storage& statistics_storage, StatisticsDomain domain)
    : domain_(domain) {
  statistics_holder_ = statistics_storage.RegisterWriter(
      fmt::format("grpc.{}", ToString(domain)),
      [this](utils::statistics::Writer& writer) { ExtendStatistics(writer); });
}

StatisticsStorage::~StatisticsStorage() { statistics_holder_.Unregister(); }

ugrpc::impl::ServiceStatistics& StatisticsStorage::GetServiceStatistics(
    const ugrpc::impl::StaticServiceMetadata& metadata) {
  // We exploit the fact that 'service_full_name' always points to the same
  // static string for a given service.
  const ServiceId service_id = metadata.service_full_name.data();

  {
    const std::shared_lock lock(mutex_);
    if (auto* stats = utils::FindOrNullptr(service_statistics_, service_id)) {
      return *stats;
    }
  }

  // All the other clients are blocked while we instantiate stats for a new
  // service. This is OK, because it will only happen a finite number of times
  // during startup.
  const std::lock_guard lock(mutex_);

  const auto [iter, is_new] =
      service_statistics_.try_emplace(service_id, metadata, domain_);
  return iter->second;
}

void StatisticsStorage::ExtendStatistics(utils::statistics::Writer& writer) {
  const std::shared_lock lock(mutex_);
  {
    auto by_destination = writer["by-destination"];
    for (const auto& [_, service_stats] : service_statistics_) {
      by_destination = service_stats;
    }
  }
}

std::uint64_t StatisticsStorage::GetStartedRequests() const {
  std::uint64_t result{0};
  // mutex_ is not locked as we might be inside a common thread w/o coroutine
  // environment. service_statistics_ is not changed as all gRPC services (not
  // clients!) are already registered.
  for (const auto& [name, stats] : service_statistics_) {
    result += stats.GetStartedRequests();
  }
  return result;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
