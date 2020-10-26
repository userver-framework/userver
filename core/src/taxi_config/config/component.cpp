#include <clients/http/component.hpp>

#include <fstream>

#include <formats/json.hpp>
#include <taxi_config/configs/component.hpp>

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
TaxiConfigClient::TaxiConfigClient(const ComponentConfig& config,
                                   const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  clients::taxi_config::ClientConfig client_config;
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

  if (client_config.use_uconfigs &&
      client_config.get_configs_overrides_for_service) {
    throw std::logic_error(
        "Cannot get configs overrides for service with `uconfigs` yet");
  }

  config_client_ = std::make_unique<clients::taxi_config::Client>(
      context.FindComponent<HttpClient>().GetHttpClient(), client_config);
}

clients::taxi_config::Client& TaxiConfigClient::GetClient() const {
  return *config_client_;
}

}  // namespace components
