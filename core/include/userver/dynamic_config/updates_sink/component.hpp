#pragma once

/// @file userver/dynamic_config/updates_sink/component.hpp
/// @brief @copybrief components::DynamicConfigUpdatesSinkBase

#include <memory>
#include <string_view>

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class DynamicConfigUpdatesSinkBase;
}  // namespace components

namespace dynamic_config::impl {
components::DynamicConfigUpdatesSinkBase& FindUpdatesSink(
    const components::ComponentConfig&, const components::ComponentContext&);
}  // namespace dynamic_config::impl

namespace components {

/// @ingroup userver_base_classes
///
/// @brief Base class for components acting as dynamic config updates sinks.
///
/// Dynamic config updaters (see components::DynamicConfigClientUpdater for
/// example) internally look for updates sink object using
/// dynamic_config::FindUpdatesSink function and use it to store received
/// updates to the dynamic config. By default, this function returns
/// components::DynamicConfig itself, however some user-defined components may
/// inherit from components::DynamicConfigUpdatesSinkBase to apply various
/// transformations to parameters before storing them to dynamic config.
///
/// @warning Unless explicitly stated, descendants of this class expect that
/// `SetConfig` method calls are serialized.
class DynamicConfigUpdatesSinkBase : public LoggableComponentBase {
 public:
  DynamicConfigUpdatesSinkBase(const components::ComponentConfig&,
                               const components::ComponentContext&);

  ~DynamicConfigUpdatesSinkBase() override;

  /// Called by updaters to store new version of dynamic config.
  /// @param updater updater name used for logging
  /// @param config updated dynamic config
  virtual void SetConfig(std::string_view updater,
                         dynamic_config::DocsMap&& config) = 0;

  /// Called by updaters to store new version of dynamic config.
  /// Default implementation creates a copy of `config` and then uses it
  /// in the call to `SetConfig` overload with an rvalue `DocsMap` parameter.
  /// @param updater updater name used for logging
  /// @param config updated dynamic config
  virtual void SetConfig(std::string_view updater,
                         const dynamic_config::DocsMap& config);

  /// Called when updater failed to load dynamic config.
  /// When service is started and dynamic config cache file is not found,
  /// components::DynamicConfig expects updater to provide current version of
  /// the dynamic config and blocks all reads until it is received. Updater is
  /// expected to call this method if it can't load dynamic config.
  /// @param updater updater name used for logging
  /// @param error error to be outputted to log
  virtual void NotifyLoadingFailed(std::string_view updater,
                                   std::string_view error) = 0;

 private:
  struct UsedByInfo;

  friend components::DynamicConfigUpdatesSinkBase&
  dynamic_config::impl::FindUpdatesSink(const components::ComponentConfig&,
                                        const components::ComponentContext&);

  std::unique_ptr<UsedByInfo> used_by_;
};

}  // namespace components

USERVER_NAMESPACE_END
