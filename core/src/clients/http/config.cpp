#include <clients/http/config.hpp>

namespace clients {
namespace http {

Config::Config(const DocsMap& docs_map)
    : threads("HTTP_CLIENT_THREADS", docs_map),
      connection_pool_size("HTTP_CLIENT_CONNECTION_POOL_SIZE", docs_map),
      connect_throttle_max_size_(
          docs_map.Get("HTTP_CLIENT_CONNECT_THROTTLE")["max-size"]
              .As<size_t>()),
      connect_throttle_update_interval_(
          docs_map
              .Get("HTTP_CLIENT_CONNECT_THROTTLE")["token-update-interval-ms"]
              .As<size_t>()) {}

}  // namespace http
}  // namespace clients
