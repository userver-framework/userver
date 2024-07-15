#include <userver/server/request/task_inherited_request.hpp>

#include <server/http/http_request_impl.hpp>
#include <server/request/task_inherited_request_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
const std::string kEmptyHeader{};

template <typename Header>
const std::string& DoGetTaskInheritedHeader(const Header& header_name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return kEmptyHeader;
  }
  return (*request)->GetHeader(header_name);
}

template <typename Header>
bool DoHasTaskInheritedHeader(const Header& header_name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return false;
  }
  return (*request)->HasHeader(header_name);
}

}  // namespace

const std::string& GetTaskInheritedHeader(std::string_view header_name) {
  return DoGetTaskInheritedHeader(header_name);
}

const std::string& GetTaskInheritedHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
  return DoGetTaskInheritedHeader(header_name);
}

bool HasTaskInheritedHeader(std::string_view header_name) {
  return DoHasTaskInheritedHeader(header_name);
}

bool HasTaskInheritedHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
  return DoHasTaskInheritedHeader(header_name);
}

}  // namespace server::request

USERVER_NAMESPACE_END
