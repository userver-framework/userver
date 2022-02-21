#pragma once

/// @file userver/clients/config/client.hpp
/// @brief @copybrief clients::dynamic_config::Client

#include <chrono>
#include <optional>

#include <userver/dynamic_config/value.hpp>
#include <userver/taxi_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace clients::dynamic_config {

struct ClientConfig {
  std::string service_name;
  bool get_configs_overrides_for_service{true};
  std::chrono::milliseconds timeout{0};
  int retries{1};
  std::string config_url;
  std::string stage_name;
  bool use_uconfigs{false};
  bool fallback_to_no_proxy{true};
};

/// @ingroup userver_clients
///
/// @brief Client for the configs service.
///
/// It is safe to concurrently invoke members of the same client because this
/// client is a thin wrapper around clients::http::Client.
class Client final {
 public:
  Client(clients::http::Client& http_client, const ClientConfig&);

  ~Client();

  using Timestamp = std::string;

  struct Reply {
    USERVER_NAMESPACE::dynamic_config::DocsMap docs_map;
    Timestamp timestamp;
  };

  struct JsonReply {
    formats::json::Value configs;
    Timestamp timestamp;
  };

  enum class Source { kConfigs, kUconfigs };

  Reply DownloadFullDocsMap();

  Reply FetchDocsMap(const std::optional<Timestamp>& last_update,
                     const std::vector<std::string>& fields_to_load);

  JsonReply FetchJson(const std::optional<Timestamp>& last_update,
                      const std::unordered_set<std::string>& fields_to_load);

 private:
  formats::json::Value FetchConfigs(
      const std::optional<Timestamp>& last_update,
      formats::json::ValueBuilder&& fields_to_load, Source source);

  std::string FetchConfigsValues(const std::string& body);

 private:
  const ClientConfig config_;
  clients::http::Client& http_client_;
};

}  // namespace clients::dynamic_config

namespace clients::taxi_config {

using Client = clients::dynamic_config::Client;

}  // namespace clients::taxi_config

USERVER_NAMESPACE_END
