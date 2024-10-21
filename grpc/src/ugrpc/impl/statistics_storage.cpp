#include <userver/ugrpc/impl/statistics_storage.hpp>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>
#include <boost/pfr/ops_fields.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

std::string_view ToString(StatisticsDomain domain) {
    static constexpr utils::TrivialBiMap kMap = [](auto selector) {
        return selector().Case(StatisticsDomain::kClient, "client").Case(StatisticsDomain::kServer, "server");
    };
    return utils::impl::EnumToStringView(domain, kMap);
}

StatisticsStorage::StatisticsStorage(utils::statistics::Storage& statistics_storage, StatisticsDomain domain)
    : domain_(domain) {
    statistics_holder_ = statistics_storage.RegisterWriter(
        fmt::format("grpc.{}", ToString(domain)),
        [this](utils::statistics::Writer& writer) { ExtendStatistics(writer); }
    );
}

StatisticsStorage::~StatisticsStorage() { statistics_holder_.Unregister(); }

ServiceStatistics&
StatisticsStorage::GetServiceStatistics(const StaticServiceMetadata& metadata, std::optional<std::string> client_name) {
    // We exploit the fact that 'service_full_name' always points to the same
    // static string for a given service.
    const ServiceId service_id = metadata.service_full_name.data();
    ServiceKey service_key{service_id, std::move(client_name)};

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

    const auto [iter, is_new] =
        service_statistics->try_emplace(std::move(service_key), metadata, domain_, global_started_);
    return iter->second;
}

MethodStatistics&
StatisticsStorage::GetGenericStatistics(std::string_view call_name, std::optional<std::string_view> client_name) {
    const GenericKeyView generic_key{call_name, client_name};

    {
        auto generic_statistics = generic_statistics_map_.SharedMutableLockUnsafe();
        if (auto* stats = utils::impl::FindTransparentOrNullptr(*generic_statistics, generic_key)) {
            return *stats;
        }
    }

    // All the other clients are blocked while we instantiate stats for a new
    // service. This is OK, because it will only happen a finite number of times
    // during startup.
    auto generic_statistics = generic_statistics_map_.Lock();

    const auto [iter, is_new] = generic_statistics->try_emplace(generic_key.Dereference(), domain_, global_started_);
    return iter->second;
}

void StatisticsStorage::ExtendStatistics(utils::statistics::Writer& writer) {
    MethodStatisticsSnapshot total{domain_};

    {
        auto by_destination = writer["by-destination"];
        {
            auto service_statistics_map = service_statistics_map_.SharedLock();
            for (const auto& [key, service_stats] : *service_statistics_map) {
                service_stats.DumpAndCountTotal(by_destination, key.client_name, total);
            }
        }
        {
            auto generic_statistics_map = generic_statistics_map_.SharedLock();
            for (const auto& [key, method_stats] : *generic_statistics_map) {
                const std::string_view call_name = key.call_name;

                const auto slash_pos = call_name.find('/');
                if (slash_pos == std::string_view::npos || slash_pos == 0) {
                    UASSERT_MSG(false, fmt::format("Generic RPC name '{}' does not contain /", call_name));
                    continue;
                }

                const auto service_name = call_name.substr(0, slash_pos);

                const MethodStatisticsSnapshot snapshot{method_stats};
                total.Add(snapshot);
                DumpMetricWithLabels(by_destination, snapshot, key.client_name, call_name, service_name);
            }
        }
    }

    writer["total"] = total;
}

std::uint64_t StatisticsStorage::GetStartedRequests() const { return global_started_.Load().value; }

StatisticsStorage::GenericKey  //
StatisticsStorage::GenericKeyView::Dereference() const {
    return GenericKey{
        std::string{call_name},
        client_name ? std::make_optional(std::string{*client_name}) : std::nullopt,
    };
}

bool StatisticsStorage::ServiceKeyComparer::operator()(ServiceKey lhs, ServiceKey rhs) const {
    return boost::pfr::eq_fields(lhs, rhs);
}

std::size_t StatisticsStorage::ServiceKeyHasher::operator()(const ServiceKey& key) const noexcept {
    return boost::pfr::hash_fields(key);
}

bool StatisticsStorage::GenericKeyComparer::operator()(const GenericKey& lhs, const GenericKey& rhs) const {
    return boost::pfr::eq_fields(lhs, rhs);
}

bool StatisticsStorage::GenericKeyComparer::operator()(const GenericKeyView& lhs, const GenericKey& rhs) const {
    return boost::pfr::eq_fields(lhs, rhs);
}

bool StatisticsStorage::GenericKeyComparer::operator()(const GenericKey& lhs, const GenericKeyView& rhs) const {
    return boost::pfr::eq_fields(lhs, rhs);
}

std::size_t StatisticsStorage::GenericKeyHasher::operator()(const GenericKey& key) const noexcept {
    return boost::pfr::hash_fields(key);
}

std::size_t StatisticsStorage::GenericKeyHasher::operator()(const GenericKeyView& key) const noexcept {
    return boost::pfr::hash_fields(key);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
