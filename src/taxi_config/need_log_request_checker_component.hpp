#pragma once

#include <server/request/need_log_request_checker_base_component.hpp>
#include <taxi_config/value.hpp>

#include "component.hpp"

namespace components {

template <typename Config>
class NeedLogRequestChecker : public NeedLogRequestCheckerBase {
 public:
  NeedLogRequestChecker(const components::ComponentConfig&,
                        const components::ComponentContext& context)
      : taxi_config_component_(
            context.FindComponent<components::TaxiConfig<Config>>()) {}
  virtual ~NeedLogRequestChecker() {}

  bool NeedLogRequest() const override;
  bool NeedLogRequestHeaders() const override;

 private:
  const components::TaxiConfig<Config>* taxi_config_component_ = nullptr;
};

template <typename Config>
bool NeedLogRequestChecker<Config>::NeedLogRequest() const {
  auto taxi_config = taxi_config_component_->Get();
  return taxi_config->need_log_request;
}

template <typename Config>
bool NeedLogRequestChecker<Config>::NeedLogRequestHeaders() const {
  auto taxi_config = taxi_config_component_->Get();
  return taxi_config->need_log_request_headers;
}

}  // namespace components
