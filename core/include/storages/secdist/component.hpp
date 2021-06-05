#pragma once

/// @file storages/secdist/componnet.hpp
/// @brief @copybrief components::Secdist

#include <string>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>

#include "secdist.hpp"

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Component that stores security related data (keys, passwords, ...).
///
/// The component must be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// config | path to the config file with data | -
/// missing-ok | do not terminate components load if no file found by the config option | false

// clang-format on

class Secdist final : public LoggableComponentBase {
 public:
  static constexpr const char* kName = "secdist";

  Secdist(const ComponentConfig&, const ComponentContext&);

  const storages::secdist::SecdistConfig& Get() const;

 private:
  storages::secdist::SecdistConfig secdist_config_;
};

}  // namespace components
