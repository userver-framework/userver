#include "endpoint_info.hpp"

namespace server {
namespace net {

EndpointInfo::EndpointInfo(const ListenerConfig& listener_config_,
                           request_handling::RequestHandler& request_handler_)
    : listener_config(listener_config_),
      request_handler(request_handler_),
      connection_count(0) {}

}  // namespace net
}  // namespace server
