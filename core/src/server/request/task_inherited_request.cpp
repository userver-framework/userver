#include <userver/server/request/task_inherited_request.hpp>

#include <server/http/http_request_impl.hpp>
#include <server/request/task_inherited_request_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
inline engine::TaskInheritedVariable<HeadersToPropagate> kTaskInheritedHeaders;

const std::string kEmptyHeader{};
const std::string kEmptyParameter{};
const HeadersToPropagate kEmptyHeaders;

template <typename Header>
const std::string& DoGetTaskInheritedHeader(const Header& header_name) {
  const auto* headers = kTaskInheritedHeaders.GetOptional();
  if (headers == nullptr) {
    return kEmptyHeader;
  }
  auto it = headers->find(header_name);
  if (it == headers->end()) return kEmptyHeader;
  return it->second;
}

template <typename Header>
bool DoHasTaskInheritedHeader([[maybe_unused]] const Header& header_name) {
  const auto* headers = kTaskInheritedHeaders.GetOptional();
  if (headers == nullptr) {
    return false;
  }
  return headers->contains(header_name);
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

boost::iterator_range<HeadersToPropagate::const_iterator>
GetTaskInheritedHeaders() {
  const auto* headers = kTaskInheritedHeaders.GetOptional();
  if (headers == nullptr) {
    return kEmptyHeaders;
  }
  return *headers;
}

void SetTaskInheritedHeaders(HeadersToPropagate headers) {
  kTaskInheritedHeaders.Set(std::move(headers));
}

const std::string& GetTaskInheritedQueryParameter(std::string_view name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return kEmptyParameter;
  }
  return (*request)->GetArg(name);
}

bool HasTaskInheritedQueryParameter(std::string_view name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return false;
  }
  return (*request)->HasArg(name);
}

}  // namespace server::request

USERVER_NAMESPACE_END
