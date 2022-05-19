#include "tasks.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/testsuite/testpoint.hpp>

namespace tests::handlers {

TasksSample::TasksSample(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : userver::components::LoggableComponentBase(config, context) {
  /// [register]
  auto& testsuite_tasks = userver::testsuite::GetTestsuiteTasks(context);
  // Only register task for testsuite environment
  if (testsuite_tasks.IsEnabled()) {
    testsuite_tasks.RegisterTask("sample-task", [] {
      TESTPOINT("sample-task/action", [] {
        userver::formats::json::ValueBuilder builder;
        return builder.ExtractValue();
      }());
    });
  } else {
    // Proudction code goes here
  }
  /// [register]
}

}  // namespace tests::handlers
