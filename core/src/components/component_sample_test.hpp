/// [Sample user component header]
#pragma once

#include <components/loggable_component_base.hpp>
#include <taxi_config/config_ptr.hpp>

namespace myservice::smth {

class Component final : public components::LoggableComponentBase {
 public:
  // name of your component to refer in static config
  static constexpr auto kName = "smth";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  int DoSomething() const;

  ~Component();

 private:
  taxi_config::Source config_;
};

}  // namespace myservice::smth

/// [Sample user component header]
