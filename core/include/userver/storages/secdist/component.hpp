#pragma once

/// @file storages/secdist/componnet.hpp
/// @brief @copybrief components::Secdist

#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>

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
/// config | path to the config file with data | ''
/// missing-ok | do not terminate components load if no file found by the config option | false
/// environment-secrets-key | name of environment variable from which to load additional data | -

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
