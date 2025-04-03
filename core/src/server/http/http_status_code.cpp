#include <userver/http/status_code.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

std::string_view HttpStatusString(HttpStatus status) { return StatusCodeString(status); }

}  // namespace server::http

USERVER_NAMESPACE_END
