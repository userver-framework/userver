#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

enum class EvPoolType {
  /// With this type `Client` with use the default frameworks ev-pool
  kShared,
  /// With this mode `Client` will create ev-pool and use it
  kOwned
};

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

  AuthSettings();

  AuthSettings(const formats::json::Value& secdist_doc);
};

struct RabbitEndpoints final {
  AuthSettings auth{};

  std::vector<EndpointInfo> endpoints{};

  RabbitEndpoints();

  RabbitEndpoints(const formats::json::Value& secdist_doc);
};

struct ClientSettings final {
  /// Whether client should use the default ev thread-pool or create a new one
  EvPoolType ev_pool_type = EvPoolType::kOwned;

  /// How many ev-threads should `Client` create.
  /// Ignored if EvPoolType::kShared is used
  size_t thread_count = 2;

  /// Library will create this number of connections per ev-thread
  ///
  /// You shouldn't set this value too high: 1 is likely enough
  /// for reliable networks, however if your tcp breaks quiet often increasing
  /// this value might reduce latency/error-rate.
  size_t connections_per_thread = 1;

  // TODO : channels should be pooled in a more sophisticated way,
  // as of now channel churn is possible with high concurrency rate
  /// How many channels should library create per connection for each supported
  /// channel type (`basic` | `publisher-confirms` at the time of writing).
  size_t channels_per_connection = 10;

  /// Whether to use TLS for connections
  bool secure = false;

  RabbitEndpoints endpoints{};

  ClientSettings();

  ClientSettings(const components::ComponentConfig& config,
                 const RabbitEndpoints& rabbit_endpoints);
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