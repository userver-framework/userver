#include "endpoint_info.hpp"

namespace server {
namespace net {

EndpointInfo::EndpointInfo(const ListenerConfig& listener_config_,
                           RequestHandlers& request_handlers_)
    : listener_config(listener_config_),
      request_handlers(request_handlers_),
      connection_count(0) {}

}  // namespace net
}  // namespace server
