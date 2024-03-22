#include <server/middlewares/handler_metrics.hpp>

#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/request/internal_request_context.hpp>

#include <userver/server/request/request_context.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

HandlerMetrics::HandlerMetrics(const handlers::HttpHandlerBase& handler)
    : handler_{handler} {}

void HandlerMetrics::HandleRequest(http::HttpRequest& request,
                                   request::RequestContext& context) const {
  handlers::HttpHandlerStatisticsScope stats_scope(
      handler_.GetHandlerStatistics(), request.GetMethod(),
      request.GetHttpResponse());

  const utils::FastScopeGuard dp_cancelled_scope{[&stats_scope,
                                                  &context]() noexcept {
    if (context.GetInternalContext().GetDPContext().IsCancelledByDeadline()) {
      stats_scope.OnCancelledByDeadline();
    }
  }};

  Next(request, context);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
