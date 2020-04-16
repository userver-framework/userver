#include <clients/http/component.hpp>
#include <taxi_config/configs/component.hpp>

namespace components {
TaxiConfigClient::TaxiConfigClient(const ComponentConfig& config,
                                   const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  clients::taxi_config::ClientConfig client_config;
  client_config.timeout =
      utils::StringToDuration(config.ParseString("http-timeout"));
  client_config.retries = config.ParseInt("http-retries");
  client_config.config_url = config.ParseString("config-url");
  config_client_ = std::make_unique<clients::taxi_config::Client>(
      context.FindComponent<HttpClient>().GetHttpClient(), client_config);
}

clients::taxi_config::Client& TaxiConfigClient::GetClient() const {
  return *config_client_;
}

}  // namespace components
