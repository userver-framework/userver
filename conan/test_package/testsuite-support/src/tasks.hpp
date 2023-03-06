#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace tests::handlers {

class TasksSample final : public components::LoggableComponentBase {
 public:
  static constexpr auto kName = "tasks-sample";

  TasksSample(const components::ComponentConfig&,
              const components::ComponentContext&);
};

}  // namespace tests::handlers
