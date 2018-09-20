#pragma once

#include <mutex>
#include <vector>

#include <components/updating_component_base.hpp>
#include "http_handler_json_base.hpp"

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

  void RegisterCacheInvalidator(components::UpdatingComponentBase& owner,
                                std::function<void()>&& handler);

  void UnregisterCacheInvalidator(components::UpdatingComponentBase& owner);

 private:
  struct Invalidator {
    components::UpdatingComponentBase* owner;
    std::function<void()> handler;

    Invalidator(components::UpdatingComponentBase* owner,
                std::function<void()>&& handler)
        : owner(owner), handler(std::move(handler)) {}
  };

  std::vector<Invalidator> cache_invalidators_;
  mutable std::mutex mutex_;
};

}  // namespace handlers
}  // namespace server
