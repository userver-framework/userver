#include <userver/server/handlers/dns_client_control.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

DnsClientControl::DnsClientControl(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : HttpHandlerBase{config, context, /*is_monitor=*/true},
      resolver_{
          &context.FindComponent<clients::dns::Component>().GetResolver()} {}

std::string DnsClientControl::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext&) const {
  UASSERT(resolver_);
  const auto command = request.GetPathArg("command");
  if (command == "reload_hosts") {
    resolver_->ReloadHosts();
  } else if (command == "flush_cache") {
    const auto name = request.GetArg("name");
    if (name.empty()) {
      request.SetResponseStatus(http::HttpStatus::kBadRequest);
      return "Missing or empty name parameter";
    }
    resolver_->FlushNetworkCache(name);
  } else if (command == "flush_cache_full") {
    resolver_->FlushNetworkCache();
  } else {
    request.SetResponseStatus(http::HttpStatus::kNotFound);
    return "Unsupported command";
  }
  return "OK";
}

yaml_config::Schema DnsClientControl::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-dns-client-control config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
