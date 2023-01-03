#pragma once

#include <mutex>

#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class Manager;
}

namespace server::handlers {

class CoroDebug final : public HttpHandlerBase {
 public:
  static constexpr std::string_view kName{"handler-coro-debug"};

  CoroDebug(const components::ComponentConfig& config,
            const components::ComponentContext& context);
  ~CoroDebug() override;

  std::string HandleRequestThrow(const http::HttpRequest&,
                                 request::RequestContext&) const final;

 private:
  void StopTheWorld() const;
  void CollectDebugInfo(std::string& result) const;
  void ResumeTheWorld() const;

  const components::Manager& manager_;

  mutable std::mutex processing_mutex_;
  mutable std::atomic<bool> do_yield_{false};
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
