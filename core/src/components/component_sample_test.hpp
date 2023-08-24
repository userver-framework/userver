#include <userver/utest/using_namespace_userver.hpp>

/// [Sample user component header]
#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>

namespace myservice::smth {

class Component final : public components::LoggableComponentBase {
 public:
  // name of your component to refer in static config
  static constexpr std::string_view kName = "smth";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  int DoSomething() const;

  ~Component() final;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  dynamic_config::Source config_;
};

}  // namespace myservice::smth

/// [Sample user component header]

/// [Sample kHasValidate specialization]
template <>
inline constexpr bool components::kHasValidate<myservice::smth::Component> =
    true;
/// [Sample kHasValidate specialization]

/// [Sample kConfigFileMode specialization]
template <>
inline constexpr auto components::kConfigFileMode<myservice::smth::Component> =
    ConfigFileMode::kNotRequired;
/// [Sample kConfigFileMode specialization]
