#include <clients/http/component.hpp>

#include <clients/http/client.hpp>
#include <clients/http/config.hpp>

namespace components {

HttpClient::HttpClient(const ComponentConfig& component_config,
                       const ComponentContext& context)

    : LoggableComponentBase(component_config, context),
      taxi_config_component_(context.FindComponent<components::TaxiConfig>()) {
  auto config = taxi_config_component_.Get();
  const auto& http_config = config->Get<clients::http::Config>();
  size_t threads = http_config.threads;

  http_client_ = std::make_unique<clients::http::Client>(threads);

  OnConfigUpdate(config);

  subscriber_scope_ =
      taxi_config_component_.AddListener(this, &HttpClient::OnConfigUpdate);
}

HttpClient::~HttpClient() { subscriber_scope_.Unsubscribe(); }

clients::http::Client& HttpClient::GetHttpClient() {
  if (!http_client_) {
    LOG_ERROR() << "Asking for http client after components::HttpClient "
                   "destructor is called.";
    logging::LogFlush();
    abort();
  }
  return *http_client_;
}

void HttpClient::OnConfigUpdate(
    const std::shared_ptr<taxi_config::Config>& config) {
  const auto& http_client_config = config->Get<clients::http::Config>();
  http_client_->SetConnectionPoolSize(http_client_config.connection_pool_size);
}

}  // namespace components
