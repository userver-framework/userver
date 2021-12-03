#include "component_sample_test.hpp"

/// [Sample user component source]
#include <userver/taxi_config/storage/component.hpp>

#include <userver/components/component.hpp>

namespace myservice::smth {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      config_(
          // searching for some component to initialize members
          context
              .FindComponent<components::TaxiConfig>()
              // getting "client" from a component
              .GetSource()) {
  // Reading config values from static config
  [[maybe_unused]] auto url = config["some-url"].As<std::string>();

  // ...
}

}  // namespace myservice::smth
/// [Sample user component source]

namespace myservice::smth {

Component::~Component() = default;

}  // namespace myservice::smth

/// [Sample user component runtime config source]
namespace myservice::smth {

namespace {
int ParseRuntimeCfg(const taxi_config::DocsMap& docs_map) {
  return docs_map.Get("SAMPLE_INTEGER_FROM_RUNTIME_CONFIG").As<int>();
}
constexpr taxi_config::Key<ParseRuntimeCfg> kMyConfig{};
}  // namespace

int Component::DoSomething() const {
  // Getting a snapsot of runtime config.
  const auto runtime_config = config_.GetSnapshot();
  return runtime_config[kMyConfig];
}

}  // namespace myservice::smth
/// [Sample user component runtime config source]
