#pragma once

#include <userver/components/component_fwd.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class Logging;
}  // namespace components

namespace testsuite::impl::actions {

class LogCapture final : public BaseTestsuiteAction {
 public:
  LogCapture(const components::ComponentContext& component_context);

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  components::Logging& logging_component_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
