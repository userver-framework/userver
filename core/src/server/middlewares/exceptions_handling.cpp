#include <server/middlewares/exceptions_handling.hpp>

#include <server/http/http_request_impl.hpp>
#include <server/request/internal_request_context.hpp>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/formatted_error_data.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

ExceptionsHandling::ExceptionsHandling(const handlers::HttpHandlerBase& handler)
    : handler_{handler} {}

void ExceptionsHandling::HandleRequest(http::HttpRequest& request,
                                       request::RequestContext& context) const {
  try {
    Next(request, context);
  } catch (const handlers::CustomHandlerException& ex) {
    handler_.HandleCustomHandlerException(request, ex);
  } catch (const std::exception& ex) {
    handler_.HandleUnknownException(request, ex);
  }
}

UnknownExceptionsHandling::UnknownExceptionsHandling(
    const handlers::HttpHandlerBase& handler)
    : handler_{handler} {}

void UnknownExceptionsHandling::HandleRequest(
    http::HttpRequest& request, request::RequestContext& context) const {
  try {
    Next(request, context);
  } catch (const std::exception& ex) {
    handler_.HandleUnknownException(request, ex);
  } catch (...) {
    LOG_WARNING() << "unknown exception in '" << handler_.HandlerName()
                  << "' handler (task cancellation?)";
    request.GetHttpResponse().SetStatus(http::HttpStatus::kClientClosedRequest);
  }
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
