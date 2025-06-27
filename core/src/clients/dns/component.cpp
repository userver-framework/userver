#include <userver/clients/dns/component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/static_config/dns_client_parsers.ipp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
namespace {
constexpr std::string_view kDnsReplySource = "dns_reply_source";
}  // namespace

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase{config, context},
      resolver_{GetFsTaskProcessor(config, context), config.As<::userver::static_config::DnsClient>()} {
    auto& storage = context.FindComponent<components::StatisticsStorage>().GetStorage();
    statistics_holder_ = storage.RegisterWriter(config.Name() + ".replies", [this](auto& writer) { Write(writer); });
}

Resolver& Component::GetResolver() { return resolver_; }

void Component::Write(utils::statistics::Writer& writer) {
    const auto& counters = GetResolver().GetLookupSourceCounters();
    writer.ValueWithLabels(counters.file, {kDnsReplySource, "file"});
    writer.ValueWithLabels(counters.cached, {kDnsReplySource, "cached"});
    writer.ValueWithLabels(counters.cached_stale, {kDnsReplySource, "cached-stale"});
    writer.ValueWithLabels(counters.cached_failure, {kDnsReplySource, "cached-failure"});
    writer.ValueWithLabels(counters.network, {kDnsReplySource, "network"});
    writer.ValueWithLabels(counters.network_failure, {kDnsReplySource, "network-failure"});
}

}  // namespace clients::dns

USERVER_NAMESPACE_END
