#pragma once

#include <userver/testsuite/testsuite_support.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class Logging;
class TestsuiteSupport;
}  // namespace components

namespace testsuite::impl::actions {

class Control final : public BaseTestsuiteAction {
 public:
  Control(const components::ComponentContext& component_context,
          bool testpoint_supported);

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  components::TestsuiteSupport& testsuite_support_;
  components::Logging& logging_component_;
  const bool testpoint_supported_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
