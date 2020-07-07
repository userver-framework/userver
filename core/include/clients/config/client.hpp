#pragma once

#include <chrono>
#include <optional>

#include <taxi_config/value.hpp>

namespace clients {
namespace http {
class Client;
}
}  // namespace clients

namespace clients {
namespace taxi_config {

struct ClientConfig {
  std::chrono::milliseconds timeout{0};
  int retries{1};
  std::string config_url;
  std::string stage_name;
  bool use_uconfigs;
};

class Client final {
 public:
  Client(clients::http::Client& http_client, const ClientConfig&);

  ~Client();

  using Timestamp = std::string;

  struct Reply {
    ::taxi_config::DocsMap docs_map;
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

}  // namespace taxi_config
}  // namespace clients
