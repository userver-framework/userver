#pragma once

/// @file userver/server/handlers/dns_client_control.hpp
/// @brief @copybrief server::handlers::DnsClientControl

#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Resolver;
}  // namespace clients::dns

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handlers that controls the DNS client.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp Sample handler dns client control component config
///
/// ## Schema
/// Set an URL path argument `command` to one of the following values:
/// * `reload_hosts` - to reload hosts file cache
/// * `flush_cache` - to remove network cache records for `name` specified as a query parameter
/// * `flush_cache_full` - to completely wipe network cache

// clang-format on

class DnsClientControl final : public HttpHandlerBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::DnsClientControl
  static constexpr std::string_view kName = "handler-dns-client-control";

  DnsClientControl(const components::ComponentConfig&,
                   const components::ComponentContext&);

  std::string HandleRequestThrow(const http::HttpRequest&,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  clients::dns::Resolver* resolver_;
};

}  // namespace server::handlers

template <>
inline constexpr bool
    components::kHasValidate<server::handlers::DnsClientControl> = true;

USERVER_NAMESPACE_END
