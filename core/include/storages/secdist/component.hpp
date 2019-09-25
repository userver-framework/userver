#pragma once

#include <string>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>

#include "secdist.hpp"

namespace components {

class Secdist final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "secdist";

  Secdist(const ComponentConfig&, const ComponentContext&);

  const storages::secdist::SecdistConfig& Get() const;

 private:
  storages::secdist::SecdistConfig secdist_config_;
};

}  // namespace components
