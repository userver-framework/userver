#include "component_sample_test.hpp"

/// [Sample user component source]
#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/utils/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace myservice::smth {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      config_(
          // Searching for some component to initialize members
          context.FindComponent<components::DynamicConfig>()
              .GetSource()  // getting "client" from a component
      ) {
  // Reading config values from static config
  [[maybe_unused]] auto url = config["some-url"].As<std::string>();
  const auto fs_tp_name = config["fs-task-processor"].As<std::string>();

  // Starting a task on a separate task processor from config
  auto& fs_task_processor = context.GetTaskProcessor(fs_tp_name);
  utils::Async(fs_task_processor, "my-component/fs-work", [] { /*...*/ }).Get();
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
int ParseRuntimeCfg(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("SAMPLE_INTEGER_FROM_RUNTIME_CONFIG").As<int>();
}
constexpr dynamic_config::Key<ParseRuntimeCfg> kMyConfig{};
}  // namespace

int Component::DoSomething() const {
  // Getting a snapshot of dynamic config.
  const auto runtime_config = config_.GetSnapshot();
  return runtime_config[kMyConfig];
}

}  // namespace myservice::smth
/// [Sample user component runtime config source]

/// [Sample user component schema]
namespace myservice::smth {

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: user component smth
additionalProperties: false
properties:
    some-url:
        type: string
        description: url for something
    fs-task-processor:
        type: string
        description: name of the task processor to do some blocking FS syscalls
)");
}

}  // namespace myservice::smth
/// [Sample user component schema]
