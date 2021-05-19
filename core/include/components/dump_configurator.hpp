#pragma once

#include <string>

#include <components/component_fwd.hpp>
#include <components/loggable_component_base.hpp>

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Helper component that manages common configuration for userver dumps.
///
/// The component must be configured in service config.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// dump-root | Components store dumps in subdirectories of this directory | -
///
/// ## Config example:
///
/// @snippet components/common_component_list_test.cpp Sample dump configurator component config

// clang-format on
class DumpConfigurator final : public LoggableComponentBase {
 public:
  static constexpr auto kName = "dump-configurator";

  DumpConfigurator(const ComponentConfig& config,
                   const ComponentContext& context);

  const std::string& GetDumpRoot() const;

 private:
  const std::string dump_root_;
};

}  // namespace components
