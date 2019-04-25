#include "endpoint_info.hpp"

namespace server {
namespace net {

EndpointInfo::EndpointInfo(const ListenerConfig& listener_config,
                           http::HttpRequestHandler& request_handler)
    : listener_config(listener_config), request_handler(request_handler) {}

std::string EndpointInfo::GetDescription() const {
  if (listener_config.unix_socket_path.empty())
    return "port=" + std::to_string(listener_config.port);
  else
    return "unix_socket_path=" + listener_config.unix_socket_path;
}

}  // namespace net
}  // namespace server
