#include <userver/dynamic_config/client/client.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {
namespace {
constexpr std::string_view kConfigsValues = "/configs/values";
}  // namespace

Client::Client(clients::http::Client& http_client, const ClientConfig& config)
    : config_(config),
      http_client_(http_client)
{}

Client::~Client() = default;

std::string Client::FetchConfigsValues(std::string_view body) {
    const auto timeout_ms = config_.timeout.count();
    const auto retries = config_.retries;
    const auto
        url = config_.append_path_to_url ? utils::StrCat(config_.config_url, kConfigsValues) : config_.config_url;

    auto reply = http_client_.CreateRequest().post(url, std::string{body}).timeout(timeout_ms).retry(retries).perform();
    reply->raise_for_status();
    return std::move(*reply).body();
}

Client::Reply Client::FetchDocsMap(
    const std::optional<Timestamp>& last_update,
    const std::vector<std::string>& fields_to_load
) {
    const auto json_value = FetchConfigs(last_update, fields_to_load);

    Reply reply;
    reply.docs_map.Parse(json_value["configs"], true);
    reply.removed = json_value["removed"].As<std::vector<std::string>>({});
    reply.timestamp = json_value["updated_at"].As<std::string>();
    reply.kill_switches_disabled = json_value["kill_switches_disabled"].As<std::vector<std::string>>({});
    return reply;
}

Client::Reply Client::DownloadFullDocsMap() { return FetchDocsMap(std::nullopt, {}); }

Client::JsonReply Client::FetchJson(
    const std::optional<Timestamp>& last_update,
    const std::vector<std::string>& fields_to_load
) {
    auto json_value = FetchConfigs(last_update, fields_to_load);
    auto configs_json = json_value["configs"];

    JsonReply reply;
    reply.configs = configs_json;

    reply.timestamp = json_value["updated_at"].As<std::string>();
    reply.kill_switches_disabled = json_value["kill_switches_disabled"].As<std::vector<std::string>>({});
    return reply;
}

formats::json::Value Client::FetchConfigs(
    const std::optional<Timestamp>& last_update,
    const std::vector<std::string>& fields_to_load
) {
    formats::json::StringBuilder body;

    {
        const formats::json::StringBuilder::ObjectGuard guard{body};
        if (!fields_to_load.empty()) {
            body.Key("ids");
            WriteToStream(fields_to_load, body);
        }

        if (last_update) {
            body.Key("updated_since");
            WriteToStream(*last_update, body);
        }

        if (!config_.stage_name.empty()) {
            body.Key("stage_name");
            WriteToStream(config_.stage_name, body);
        }

        if (config_.get_configs_overrides_for_service) {
            body.Key("service");
            WriteToStream(config_.service_name, body);
        }

        if (config_.is_prestable) {
            body.Key("is_prestable");
            WriteToStream(true, body);
        }
    }

    LOG_TRACE() << "request body: " << body.GetStringView();
    auto json = FetchConfigsValues(body.GetStringView());

    return formats::json::FromString(json);
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
