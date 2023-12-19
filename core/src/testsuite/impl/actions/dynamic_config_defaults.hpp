#pragma once

#include <userver/dynamic_config/fwd.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class DynamicConfigDefaults final : public BaseTestsuiteAction {
 public:
  explicit DynamicConfigDefaults(
      const components::ComponentContext& component_context);

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  const dynamic_config::DocsMap& defaults_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
