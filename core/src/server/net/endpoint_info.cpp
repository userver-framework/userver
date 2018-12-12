#include "endpoint_info.hpp"

namespace server {
namespace net {

EndpointInfo::EndpointInfo(const ListenerConfig& listener_config,
                           http::HttpRequestHandler& request_handler)
    : listener_config(listener_config),
      request_handler(request_handler),
      connection_count(0) {}

}  // namespace net
}  // namespace server
