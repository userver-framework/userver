#pragma once

#include <clients/config/client.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>

namespace components {

class TaxiConfigClient : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "taxi-configs-client";

  TaxiConfigClient(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfigClient() override = default;

  [[nodiscard]] clients::taxi_config::Client& GetClient() const;

 private:
  std::unique_ptr<clients::taxi_config::Client> config_client_;
};

}  // namespace components
