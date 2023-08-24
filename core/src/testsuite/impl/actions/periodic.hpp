#pragma once

#include <userver/formats/json/inline.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class PeriodicTaskRun final : public BaseTestsuiteAction {
 public:
  PeriodicTaskRun(components::TestsuiteSupport& testsuite_support)
      : testsuite_support_(testsuite_support) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override {
    const auto task_name = request_body["name"].As<std::string>();
    const bool status =
        testsuite_support_.GetPeriodicTaskControl().RunPeriodicTask(task_name);
    return formats::json::MakeObject("status", status);
  }

 private:
  components::TestsuiteSupport& testsuite_support_;
};

class PeriodicTaskSuspend final : public BaseTestsuiteAction {
 public:
  PeriodicTaskSuspend(components::TestsuiteSupport& testsuite_support)
      : testsuite_support_(testsuite_support) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override {
    const auto task_names =
        request_body["names"].As<std::unordered_set<std::string>>();
    testsuite_support_.GetPeriodicTaskControl().SuspendPeriodicTasks(
        task_names);
    return {};
  }

 private:
  components::TestsuiteSupport& testsuite_support_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
