#pragma once

#include <functional>
#include <vector>

#include <cache/cache_update_trait.hpp>
#include <server/handlers/http_handler_json_base.hpp>

namespace components {
class TestsuiteSupport;
}  // namespace components

namespace server::handlers {

class TestsControl final : public HttpHandlerJsonBase {
 public:
  TestsControl(const components::ComponentConfig& config,
               const components::ComponentContext& component_context);

  static constexpr const char* kName = "tests-control";

  const std::string& HandlerName() const override;
  formats::json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request,
      const formats::json::Value& request_body,
      request::RequestContext& context) const override;

 private:
  formats::json::Value ActionRunPeriodicTask(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionSuspendPeriodicTasks(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionWriteCacheDumps(
      const formats::json::Value& request_body) const;
  formats::json::Value ActionReadCacheDumps(
      const formats::json::Value& request_body) const;

  components::TestsuiteSupport& testsuite_support_;
};

}  // namespace server::handlers
