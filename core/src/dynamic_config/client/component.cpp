#include <userver/dynamic_config/client/component.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

const std::string kStageSettingsFile = "/etc/yandex/settings.json";

std::string ReadStageName() {
  using formats::json::blocking::FromFile;
  try {
    return FromFile(kStageSettingsFile)["env_name"].As<std::string>();
  } catch (const std::exception& exception) {
    LOG_ERROR() << "Error during config service client initialization. "
                << "Config use_uconfigs set true, got error during read file: "
                << kStageSettingsFile << " error: " << exception;
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
  client_config.use_uconfigs = config["use-uconfigs"].As<bool>(false);
  client_config.timeout =
      utils::StringToDuration(config["http-timeout"].As<std::string>());
  client_config.retries = config["http-retries"].As<int>();
  if (client_config.use_uconfigs) {
    client_config.stage_name = ReadStageName();
    client_config.config_url = config["uconfigs-url"].As<std::string>();
  } else {
    client_config.config_url = config["config-url"].As<std::string>();
  }
  client_config.fallback_to_no_proxy =
      config["fallback-to-no-proxy"].As<bool>(true);

  if (client_config.use_uconfigs &&
      client_config.get_configs_overrides_for_service) {
    throw std::logic_error(
        "Cannot get configs overrides for service with `uconfigs` yet");
  }

  config_client_ = std::make_unique<dynamic_config::Client>(
      context.FindComponent<HttpClient>().GetHttpClient(), client_config);
}

dynamic_config::Client& DynamicConfigClient::GetClient() const {
  return *config_client_;
}

yaml_config::Schema DynamicConfigClient::GetStaticConfigSchema() {
  return yaml_config::Schema(R"(
type: object
description: taxi-configs-client config
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
        description: HTTP URL to request configs via POST request, ignored if use-uconfigs is true
    use-uconfigs:
        type: boolean
        description: set to true to read stage name from "/etc/yandex/settings.json" and send it in requests
        defaultDescription: false
    uconfigs-url:
        type: string
        description: HTTP URL to request configs via POST request if use-uconfigs is true
    fallback-to-no-proxy:
        type: boolean
        description: make additional attempts to retrieve configs by bypassing proxy that is set in USERVER_HTTP_PROXY runtime variable
        defaultDescription: true
)");
}

}  // namespace components

USERVER_NAMESPACE_END
