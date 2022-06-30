#pragma once

#include <userver/formats/json/inline.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class TaskRun final : public BaseTestsuiteAction {
 public:
  TaskRun(components::TestsuiteSupport& testsuite_support)
      : tasks_(testsuite_support.GetTestsuiteTasks()) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  testsuite::TestsuiteTasks& tasks_;
};

class TaskSpawn final : public BaseTestsuiteAction {
 public:
  TaskSpawn(components::TestsuiteSupport& testsuite_support)
      : tasks_(testsuite_support.GetTestsuiteTasks()) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  testsuite::TestsuiteTasks& tasks_;
};

class TaskStop final : public BaseTestsuiteAction {
 public:
  TaskStop(components::TestsuiteSupport& testsuite_support)
      : tasks_(testsuite_support.GetTestsuiteTasks()) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  testsuite::TestsuiteTasks& tasks_;
};

class TasksList final : public BaseTestsuiteAction {
 public:
  TasksList(components::TestsuiteSupport& testsuite_support)
      : tasks_(testsuite_support.GetTestsuiteTasks()) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  testsuite::TestsuiteTasks& tasks_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
