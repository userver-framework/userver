#pragma once

#include <string>

#include <yandex/taxi/userver/components/component_base.hpp>
#include <yandex/taxi/userver/components/component_config.hpp>
#include <yandex/taxi/userver/components/component_context.hpp>

#include "secdist.hpp"

namespace components {

class Secdist : public ComponentBase {
 public:
  static constexpr const char* kName = "secdist";

  Secdist(const ComponentConfig&, const ComponentContext&);

  const storages::secdist::SecdistConfig& Get() const;

 private:
  storages::secdist::SecdistConfig secdist_config_;
};

}  // namespace components
