#pragma once

#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class ResetMetrics final : public BaseTestsuiteAction {
 public:
  ResetMetrics(const components::ComponentContext& component_context);

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  utils::statistics::MetricsStoragePtr metrics_storage_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
