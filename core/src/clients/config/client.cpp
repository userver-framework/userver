#include <userver/dynamic_config/client/client.hpp>

#include <cstddef>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/error_kind.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {
namespace {

constexpr std::string_view kConfigsValues = "/configs/values";

std::string MakeConfigsValuesUrl(const ClientConfig& config, const std::string& base_url) {
    return config.append_path_to_url ? utils::StrCat(base_url, kConfigsValues) : base_url;
}

std::vector<std::string> MakeConfigsValuesUrls(const ClientConfig& config) {
    std::vector<std::string> urls;
    urls.reserve(1 + config.fallback_config_urls.size());
    urls.push_back(MakeConfigsValuesUrl(config, config.config_url));
    for (const auto& fallback_url : config.fallback_config_urls) {
        urls.push_back(MakeConfigsValuesUrl(config, fallback_url));
    }
    return urls;
}

bool IsFallbackAllowed(clients::http::ErrorKind error_kind) {
    switch (error_kind) {
        case clients::http::ErrorKind::kNetwork:
        case clients::http::ErrorKind::kTimeout:
        case clients::http::ErrorKind::kServer:
            return true;
        case clients::http::ErrorKind::kDeadlinePropagation:
        case clients::http::ErrorKind::kCancel:
        case clients::http::ErrorKind::kClient:
            return false;
    }
    UINVARIANT(false, fmt::format("Unexpected clients::http::ErrorKind {}", static_cast<int>(error_kind)));
}

}  // namespace

Client::Client(clients::http::Client& http_client, const ClientConfig& config)
    : config_(config),
      urls_(MakeConfigsValuesUrls(config_)),
      http_client_(http_client)
{
    UINVARIANT(
        config_.service_overrides.empty() || !config_.get_configs_overrides_for_service,
        "dynamic_config::ClientConfig cannot use 'service' and 'service_overrides' together; "
        "set get_configs_overrides_for_service to false when service_overrides is non-empty"
    );
    for (const auto& fallback_url : config_.fallback_config_urls) {
        UINVARIANT(!fallback_url.empty(), "Empty URL in 'fallback-config-urls'");
        UINVARIANT(
            fallback_url != config_.config_url,
            fmt::format(
                "dynamic_config::ClientConfig fallback config URL '{}' duplicates 'config-url'; "
                "remove it from 'fallback-config-urls'",
                fallback_url
            )
        );
    }
}

Client::~Client() = default;

std::string Client::FetchConfigsValues(std::string_view body) {
    UASSERT(!urls_.empty());
    for (std::size_t i = 0; i + 1 < urls_.size(); ++i) {
        try {
            return FetchConfigsValuesFrom(urls_[i], body);
        } catch (const clients::http::BaseException& ex) {
            if (!IsFallbackAllowed(ex.GetErrorKind())) {
                throw;
            }
            LOG_WARNING()
                << "Failed to fetch dynamic configs from '" << urls_[i] << "', falling back to '" << urls_[i + 1]
                << "': " << ex;
        }
    }
    return FetchConfigsValuesFrom(urls_.back(), body);
}

std::string Client::FetchConfigsValuesFrom(const std::string& url, std::string_view body) {
    auto reply =
        http_client_.CreateRequest()
            .post(url, std::string{body})
            .timeout(config_.timeout.count())
            .retry(config_.retries)
            .perform();
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
    reply.configs = std::move(configs_json);

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

        if (!config_.service_overrides.empty()) {
            body.Key("service_overrides");
            WriteToStream(config_.service_overrides, body);
        } else if (config_.get_configs_overrides_for_service) {
            body.Key("service");
            WriteToStream(config_.service_name, body);
        }

        if (config_.is_prestable) {
            body.Key("is_prestable");
            WriteToStream(true, body);
        }

        if (!config_.circuit.empty()) {
            body.Key("circuit");
            WriteToStream(config_.circuit, body);
        }
    }

    LOG_TRACE() << "request body: " << body.GetStringView();
    auto json = FetchConfigsValues(body.GetStringView());

    return formats::json::FromString(json);
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
