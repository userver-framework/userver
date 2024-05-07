#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace sample {

class TopicReaderComponent final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "sample-topic-reader";

  TopicReaderComponent(const components::ComponentConfig& config,
                       const components::ComponentContext& context);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  engine::TaskWithResult<void> read_task_;
};

}  // namespace sample
