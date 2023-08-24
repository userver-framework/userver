#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct EndpointInfo final {
  /// RabbitMQ node address (either FQDN or ip)
  std::string host = "localhost";

  /// Port to connect to
  uint16_t port = 5672;
};

struct AuthSettings final {
  /// Login to use
  std::string login = "guest";

  /// Password to use
  std::string password = "guest";

  /// RabbitMQs vhost
  std::string vhost = "/";
};

struct RabbitEndpoints final {
  /// Auth settings
  AuthSettings auth{};

  /// Endpoints to connect to
  std::vector<EndpointInfo> endpoints{};
};

struct PoolSettings final {
  /// Library will try to maintain at least this amount of connections.
  /// Note that every consumer takes a connection for himself and this limit
  /// doesn't account that
  size_t min_pool_size = 5;

  /// Library will maintain at most this amount of connections.
  /// Note that every consumer takes a connection for himself and this limit
  /// doesn't account that
  size_t max_pool_size = 10;

  /// A per-connection limit for concurrent requests waiting
  /// for response from the broker.
  /// Note: increasing this allows one to potentially increase throughput,
  /// but in case of a connection-wide error
  /// (tcp error/protocol error/write timeout) leads to a errors burst:
  /// all outstanding request will fails at once
  size_t max_in_flight_requests = 5;
};

class TestsHelper;
struct ClientSettings final {
  /// Per-host connections pool settings
  PoolSettings pool_settings{};

  /// Endpoints settings
  RabbitEndpoints endpoints{};

  /// Whether to use TLS for connections
  bool use_secure_connection = true;

  ClientSettings(const components::ComponentConfig& config,
                 const RabbitEndpoints& rabbit_endpoints);

 private:
  friend class TestsHelper;
  ClientSettings();
};

class RabbitEndpointsMulti final {
 public:
  RabbitEndpointsMulti(const formats::json::Value& doc);

  const RabbitEndpoints& Get(const std::string& name) const;

 private:
  std::unordered_map<std::string, RabbitEndpoints> endpoints_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
