#include <userver/ugrpc/impl/statistics_storage.hpp>

#include <tuple>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/impl/internal_tag.hpp>
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

ServiceStatistics& StatisticsStorage::GetServiceStatistics(
    const StaticServiceMetadata& metadata,
    std::optional<std::string> endpoint) {
  // We exploit the fact that 'service_full_name' always points to the same
  // static string for a given service.
  const ServiceId service_id = metadata.service_full_name.data();
  ServiceKey service_key{service_id, std::move(endpoint)};

  {
    auto service_statistics = service_statistics_map_.SharedMutableLockUnsafe();
    if (auto* stats = utils::FindOrNullptr(*service_statistics, service_key)) {
      return *stats;
    }
  }

  // All the other clients are blocked while we instantiate stats for a new
  // service. This is OK, because it will only happen a finite number of times
  // during startup.
  auto service_statistics = service_statistics_map_.Lock();

  const auto [iter, is_new] = service_statistics->try_emplace(
      std::move(service_key), metadata, domain_, global_started_);
  return iter->second;
}

MethodStatistics& StatisticsStorage::GetGenericStatistics(
    std::string_view call_name, std::optional<std::string_view> endpoint) {
  const GenericKeyView generic_key{call_name, endpoint};

  {
    auto generic_statistics = generic_statistics_map_.SharedMutableLockUnsafe();
    if (auto* stats = utils::impl::FindTransparentOrNullptr(*generic_statistics,
                                                            generic_key)) {
      return *stats;
    }
  }

  // All the other clients are blocked while we instantiate stats for a new
  // service. This is OK, because it will only happen a finite number of times
  // during startup.
  auto generic_statistics = generic_statistics_map_.Lock();

  const auto [iter, is_new] = generic_statistics->try_emplace(
      generic_key.Dereference(), domain_, global_started_);
  return iter->second;
}

void StatisticsStorage::ExtendStatistics(utils::statistics::Writer& writer) {
  MethodStatisticsSnapshot total{domain_};

  {
    auto by_destination = writer["by-destination"];
    {
      auto service_statistics_map = service_statistics_map_.SharedLock();
      for (const auto& [key, service_stats] : *service_statistics_map) {
        if (key.endpoint) {
          by_destination.WithLabels(
              utils::impl::InternalTag{}, {"endpoint", *key.endpoint},
              [&service_stats = service_stats,
               &total = total](utils::statistics::Writer& writer) {
                service_stats.DumpAndCountTotal(writer, total);
              });
        } else {
          service_stats.DumpAndCountTotal(by_destination, total);
        }
      }
    }
    {
      auto generic_statistics_map = generic_statistics_map_.SharedLock();
      for (const auto& [key, method_stats] : *generic_statistics_map) {
        const std::string_view call_name = key.call_name;

        const auto slash_pos = call_name.find('/');
        if (slash_pos == std::string_view::npos || slash_pos == 0) {
          UASSERT(false);
          continue;
        }

        const auto service_name = call_name.substr(0, slash_pos);
        const auto method_name = call_name.substr(slash_pos + 1);

        const MethodStatisticsSnapshot snapshot{method_stats};
        total.Add(snapshot);

        if (key.endpoint) {
          by_destination.ValueWithLabels(snapshot,
                                         {{"grpc_service", service_name},
                                          {"grpc_method", method_name},
                                          {"grpc_destination", key.call_name},
                                          {"endpoint", *key.endpoint}});
        } else {
          by_destination.ValueWithLabels(snapshot,
                                         {{"grpc_service", service_name},
                                          {"grpc_method", method_name},
                                          {"grpc_destination", key.call_name}});
        }
      }
    }
  }

  // Just writing grpc.server.total metrics for now
  if (domain_ == StatisticsDomain::kServer) {
    writer["total"] = total;
  }
}

std::uint64_t StatisticsStorage::GetStartedRequests() const {
  return global_started_.Load().value;
}

StatisticsStorage::GenericKey  //
StatisticsStorage::GenericKeyView::Dereference() const {
  return GenericKey{
      std::string{call_name},
      endpoint ? std::make_optional(std::string{*endpoint}) : std::nullopt,
  };
}

bool StatisticsStorage::ServiceKeyComparer::operator()(ServiceKey lhs,
                                                       ServiceKey rhs) const {
  return lhs.service_id == rhs.service_id && lhs.endpoint == rhs.endpoint;
}

std::size_t StatisticsStorage::ServiceKeyHasher::operator()(
    const ServiceKey& key) const noexcept {
  return boost::hash_value(std::tie(key.service_id, key.endpoint));
}

bool StatisticsStorage::GenericKeyComparer::operator()(
    const GenericKey& lhs, const GenericKey& rhs) const {
  return lhs.call_name == rhs.call_name && lhs.endpoint == rhs.endpoint;
}

bool StatisticsStorage::GenericKeyComparer::operator()(
    const GenericKeyView& lhs, const GenericKey& rhs) const {
  return lhs.call_name == rhs.call_name && lhs.endpoint == rhs.endpoint;
}

bool StatisticsStorage::GenericKeyComparer::operator()(
    const GenericKey& lhs, const GenericKeyView& rhs) const {
  return lhs.call_name == rhs.call_name && lhs.endpoint == rhs.endpoint;
}

std::size_t StatisticsStorage::GenericKeyHasher::operator()(
    const GenericKey& key) const noexcept {
  return boost::hash_value(std::tie(key.call_name, key.endpoint));
}

std::size_t StatisticsStorage::GenericKeyHasher::operator()(
    const GenericKeyView& key) const noexcept {
  return boost::hash_value(std::tie(key.call_name, key.endpoint));
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
