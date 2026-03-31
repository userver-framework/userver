#include <userver/clients/dns/component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>

#include <userver/static_config/dns_client_parsers.ipp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase{config, context},
      resolver_{GetFsTaskProcessor(config, context), config.As<::userver::static_config::DnsClient>()}
{
    utils::statistics::RegisterWriterScope(context, fmt::format("{}.replies", config.Name()), [this](auto& writer) {
        resolver_.WriteMetrics(writer);
    });
}

Resolver& Component::GetResolver() { return resolver_; }

}  // namespace clients::dns

USERVER_NAMESPACE_END
