#include <userver/utest/using_namespace_userver.hpp>

/// [Sample user component header]
#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/taxi_config/source.hpp>

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
