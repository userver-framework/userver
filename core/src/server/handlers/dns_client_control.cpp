#include <userver/server/handlers/dns_client_control.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/utils/assert.hpp>

namespace server::handlers {

DnsClientControl::DnsClientControl(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : HttpHandlerBase{config, context, /*is_monitor=*/true},
      resolver_{
          &context.FindComponent<clients::dns::Component>().GetResolver()} {}

const std::string& DnsClientControl::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

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

}  // namespace server::handlers
