#pragma once

#include <vector>

#include <components/cache_update_trait.hpp>
#include <engine/mutex.hpp>
#include <server/handlers/http_handler_json_base.hpp>

namespace components {
class CacheInvalidator;
}  // namespace components

namespace server {
namespace handlers {

class TestsControl : public HttpHandlerJsonBase {
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
  components::CacheInvalidator& cache_invalidator_;
  mutable engine::Mutex mutex_;
};

}  // namespace handlers
}  // namespace server
