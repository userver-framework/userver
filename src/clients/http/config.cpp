#include <clients/http/config.hpp>

namespace clients {
namespace http {

Config::Config(const DocsMap& docs_map)
    : threads("HTTP_CLIENT_THREADS", docs_map),
      connection_pool_size("HTTP_CLIENT_CONNECTION_POOL_SIZE", docs_map) {}

}  // namespace http
}  // namespace clients
