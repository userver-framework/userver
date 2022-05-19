#pragma once

#include <userver/components/loggable_component_base.hpp>

namespace tests::handlers {

class TasksSample final : public userver::components::LoggableComponentBase {
 public:
  static constexpr auto kName = "tasks-sample";

  TasksSample(const userver::components::ComponentConfig&,
              const userver::components::ComponentContext&);
};

}  // namespace tests::handlers
