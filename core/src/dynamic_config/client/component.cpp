#include <userver/dynamic_config/client/component.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::string ReadStageName(const std::string& filepath) {
  using formats::json::blocking::FromFile;
  try {
    return FromFile(filepath)["env_name"].As<std::string>();
  } catch (const std::exception& exception) {
    LOG_ERROR() << "Error during config service client initialization. "
                << "Got error while reading stage name from file: " << filepath
                << ", error: " << exception;
    throw;
  }
}

}  // namespace

DynamicConfigClient::DynamicConfigClient(const ComponentConfig& config,
                                         const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  dynamic_config::ClientConfig client_config;
  client_config.service_name = config["service-name"].As<std::string>();
  client_config.get_configs_overrides_for_service =
      config["get-configs-overrides-for-service"].As<bool>(true);
  client_config.append_path_to_url =
      config["append-path-to-url"].As<bool>(true);
  client_config.timeout =
      utils::StringToDuration(config["http-timeout"].As<std::string>());
  client_config.retries = config["http-retries"].As<int>();
  auto stage_filepath =
      config["configs-stage-filepath"].As<std::optional<std::string>>();
  if (stage_filepath && !stage_filepath->empty()) {
    client_config.stage_name = ReadStageName(*stage_filepath);
  } else {
    auto stage_name = config["configs-stage"].As<std::optional<std::string>>();
    if (stage_name && !stage_name->empty()) {
      client_config.stage_name = *stage_name;
    }
  }
  client_config.config_url = config["config-url"].As<std::string>();
  client_config.fallback_to_no_proxy =
      config["fallback-to-no-proxy"].As<bool>(true);

  if (!client_config.stage_name.empty() &&
      client_config.get_configs_overrides_for_service) {
    throw std::logic_error(
        "Cannot get overrides for both stage and service yet");
  }

  config_client_ = std::make_unique<dynamic_config::Client>(
      context.FindComponent<HttpClient>().GetHttpClient(), client_config);
}

dynamic_config::Client& DynamicConfigClient::GetClient() const {
  return *config_client_;
}

yaml_config::Schema DynamicConfigClient::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that starts a clients::dynamic_config::Client client.
additionalProperties: false
properties:
    get-configs-overrides-for-service:
        type: boolean
        description: send service-name field
        defaultDescription: true
    service-name:
        type: string
        description: name of the service to send if the get-configs-overrides-for-service is true
    http-timeout:
        type: string
        description: HTTP request timeout to the remote in utils::StringToDuration() suitable format
    http-retries:
        type: integer
        description: string HTTP retries before reporting the request failure
    config-url:
        type: string
        description: HTTP URL to request configs via POST request
    configs-stage-filepath:
        type: string
        description: |
          file to read stage name from, overrides static "configs-stage"
          if both are provided, expected format: json file with "env_name" property
    configs-stage:
        type: string
        description: stage name provided statically, can be overridden from file
    fallback-to-no-proxy:
        type: boolean
        description: make additional attempts to retrieve configs by bypassing proxy that is set in USERVER_HTTP_PROXY runtime variable
        defaultDescription: true
)");
}

}  // namespace components

USERVER_NAMESPACE_END
