#include <userver/server/request/task_inherited_request.hpp>

#include <server/http/http_request_impl.hpp>
#include <server/request/task_inherited_request_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
const std::string kEmptyHeader{};
}  // namespace

const std::string& GetTaskInheritedHeader(const std::string& header_name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return kEmptyHeader;
  }
  return (*request)->GetHeader(header_name);
}

bool HasTaskInheritedHeader(const std::string& header_name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return false;
  }
  return (*request)->HasHeader(header_name);
}

}  // namespace server::request

USERVER_NAMESPACE_END
