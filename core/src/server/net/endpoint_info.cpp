#include "endpoint_info.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::net {

EndpointInfo::EndpointInfo(const ListenerConfig& listener_config, http::HttpRequestHandler& request_handler)
    : listener_config(listener_config), request_handler(request_handler) {}

}  // namespace server::net

USERVER_NAMESPACE_END
