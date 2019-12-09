#include <clients/config/client.hpp>

#include <clients/http/client.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

namespace clients {
namespace taxi_config {

Client::Client(clients::http::Client& http_client, const ClientConfig& config)
    : config_(config), http_client_(http_client) {}

Client::~Client() = default;

std::string Client::FetchConfigsValues(const std::string& body) {
  auto timeout_ms = config_.timeout.count();
  auto retries = config_.retries;
  auto url = config_.config_url + "/configs/values";

  auto reply = http_client_.CreateRequest()
                   ->post(url, body)
                   ->timeout(timeout_ms)
                   ->retry(retries)
                   ->perform();
  reply->RaiseForStatus(reply->status_code());

  auto json = reply->body();
  return json;
}

Client::Reply Client::FetchDocsMap(
    const boost::optional<Timestamp>& last_update,
    const std::vector<std::string>& fields_to_load) {
  formats::json::ValueBuilder body_builder(formats::json::Type::kObject);

  if (!fields_to_load.empty()) {
    formats::json::ValueBuilder fields_builder(formats::json::Type::kArray);
    for (const auto& field : fields_to_load) fields_builder.PushBack(field);
    body_builder["ids"] = std::move(fields_builder);
  }

  if (last_update) {
    body_builder["updated_since"] = *last_update;
  }

  auto request_body = formats::json::ToString(body_builder.ExtractValue());
  LOG_DEBUG() << "request body: " << request_body;

  auto json = FetchConfigsValues(request_body);

  auto json_value = formats::json::FromString(json);
  auto configs_json = json_value["configs"];

  Reply reply;
  reply.docs_map.Parse(formats::json::ToString(configs_json), true);

  auto updated_at_json = json_value["updated_at"];
  reply.timestamp = updated_at_json.As<std::string>();
  return reply;
}

Client::Reply Client::DownloadFullDocsMap() {
  return FetchDocsMap(boost::none, {});
}

}  // namespace taxi_config
}  // namespace clients
