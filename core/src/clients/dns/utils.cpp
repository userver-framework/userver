#include <userver/clients/dns/resolver_utils.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {

namespace {

ResolverType ParseDnsResolver(const components::ComponentConfig& config,
                              ResolverType default_type) {
  auto dns_resolver = config["dns_resolver"];
  if (dns_resolver.IsMissing()) return default_type;

  auto str = dns_resolver.As<std::string>();
  if (str == "getaddrinfo") return ResolverType::kGetaddrinfo;
  if (str == "async") return ResolverType::kAsync;

  throw InvalidConfigException{"Invalid value '" + str + "' for dns_resolver"};
}

}  // namespace

Resolver* GetResolverPtr(const components::ComponentConfig& config,
                         const components::ComponentContext& context,
                         ResolverType default_type) {
  if (ParseDnsResolver(config, default_type) == ResolverType::kGetaddrinfo) {
    return nullptr;
  } else {
    return &context.FindComponent<clients::dns::Component>().GetResolver();
  }
}

}  // namespace clients::dns

USERVER_NAMESPACE_END
