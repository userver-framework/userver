#include <userver/testsuite/middlewares.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

void ExceptionsHandlingMiddleware::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext& context) const {
  try {
    Next(request, context);
  } catch (const server::handlers::CustomHandlerException&) {
    throw;
  } catch (const std::exception& exc) {
    formats::json::ValueBuilder builder(formats::json::Type::kObject);
    builder["code"] = "500";
    builder["message"] = "Internal Server Error";
    builder["details"] = exc.what();

    auto& response = request.GetHttpResponse();
    response.SetStatus(server::http::HttpStatus::kInternalServerError);
    response.SetContentType(http::content_type::kApplicationJson);
    response.SetData(formats::json::ToString(builder.ExtractValue()));
  }
}

}  // namespace testsuite

USERVER_NAMESPACE_END
