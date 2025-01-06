#include <userver/server/request/task_inherited_request.hpp>

#include <algorithm>

#include <server/http/http_request_impl.hpp>
#include <server/request/task_inherited_request_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
inline engine::TaskInheritedVariable<HeadersToPropagate> kPropagatedHeaders;

const std::string kEmptyString{};
const HeadersToPropagate kEmptyHeaders;

const std::string* FindValueOrNullptr(const HeadersToPropagate& headers, std::string_view header_name) {
    const auto it = std::find_if(headers.begin(), headers.end(), [&header_name](const Header& header) {
        return header.name == header_name;
    });
    if (it == headers.end()) {
        return nullptr;
    }
    return &it->value;
}

const std::string& DoGetPropagatedHeader(std::string_view header_name) {
    const auto* headers = kPropagatedHeaders.GetOptional();
    if (headers == nullptr) {
        return kEmptyString;
    }
    const auto* const value = FindValueOrNullptr(*headers, header_name);
    if (value == nullptr) {
        return kEmptyString;
    }
    return *value;
}

bool DoHasPropagatedHeader(std::string_view header_name) {
    const auto* headers = kPropagatedHeaders.GetOptional();
    if (headers == nullptr) {
        return false;
    }
    return FindValueOrNullptr(*headers, header_name) != nullptr;
}

}  // namespace

const std::string& GetPropagatedHeader(std::string_view header_name) { return DoGetPropagatedHeader(header_name); }

const std::string& GetPropagatedHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
    return DoGetPropagatedHeader(header_name);
}

bool HasPropagatedHeader(std::string_view header_name) { return DoHasPropagatedHeader(header_name); }

bool HasPropagatedHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) {
    return DoHasPropagatedHeader(header_name);
}

boost::iterator_range<HeadersToPropagate::const_iterator> GetPropagatedHeaders() {
    const auto* headers = kPropagatedHeaders.GetOptional();
    if (headers == nullptr) {
        return kEmptyHeaders;
    }
    return *headers;
}

void SetPropagatedHeaders(HeadersToPropagate headers) { kPropagatedHeaders.Set(std::move(headers)); }

const std::string& GetTaskInheritedQueryParameter(std::string_view name) {
    const auto* request = kTaskInheritedRequest.GetOptional();
    if (request == nullptr) {
        return kEmptyString;
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
