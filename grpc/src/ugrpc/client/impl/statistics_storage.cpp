#include <userver/ugrpc/client/impl/statistics_storage.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {
constexpr std::string_view kClientPrefix = "grpc.client.";
}  // namespace

StatisticsStorage::StatisticsStorage(
    utils::statistics::Storage& statistics_storage) {
  statistics_holder_ = statistics_storage.RegisterExtender(
      {"grpc", "client"},
      [this](const utils::statistics::StatisticsRequest& request) {
        return ExtendStatistics(request.prefix);
      });
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

formats::json::Value StatisticsStorage::ExtendStatistics(
    std::string_view prefix) {
  const auto cut_prefix = prefix.size() >= kClientPrefix.size()
                              ? prefix.substr(kClientPrefix.size())
                              : std::string_view{};

  std::shared_lock lock(mutex_);

  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (const auto& [_, service_stats] : service_statistics_) {
    const auto service_name = service_stats.GetMetadata().service_full_name;
    if (utils::text::StartsWith(cut_prefix, service_name) ||
        utils::text::StartsWith(service_name, cut_prefix)) {
      result[std::string{service_name}] = service_stats.ExtendStatistics();
    }
  }
  return result.ExtractValue();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
