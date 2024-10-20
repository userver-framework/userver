#pragma once

/// @file userver/storages/secdist/component_base.hpp
/// @brief @copybrief components::SecdistComponentBase

#include <string>

#include <userver/components/component_base.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Base component that stores security related data (keys, passwords, ...).
///
/// You can use a ready-made components::Secdist or implement your own.
///
/// ### Writing your own secrets distributor:
/// Implement a custom provider class that contains the settings.
/// @snippet core/include/userver/storages/secdist/default_provider.hpp Sample secdist - default provider
///
/// Implement a custom secdist component, configure it's static config schema
/// and pass the custom provider to the storages::secdist::SecdistConfig::Settings.
/// @snippet core/include/userver/storages/secdist/component.сpp Sample secdist - default secdist
///
// clang-format on

class SecdistComponentBase : public ComponentBase {
 public:
  SecdistComponentBase(const ComponentConfig&, const ComponentContext&,
                       storages::secdist::SecdistConfig::Settings&&);

  const storages::secdist::SecdistConfig& Get() const;

  rcu::ReadablePtr<storages::secdist::SecdistConfig> GetSnapshot() const;

  storages::secdist::Secdist& GetStorage();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::secdist::Secdist secdist_;
};

}  // namespace components

USERVER_NAMESPACE_END
