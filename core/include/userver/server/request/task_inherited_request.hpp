#pragma once

/// @file userver/server/request/task_inherited_request.hpp
/// @brief Functions that provide access to HttpRequest stored in
/// TaskInheritedVariable.

#include <string>
#include <string_view>

#include <boost/container/small_vector.hpp>
#include <boost/range/iterator_range.hpp>

#include <userver/http/header_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {
class PredefinedHeader;
}  // namespace http::headers

namespace server::http {
class HttpRequestImpl;
}  // namespace server::http

namespace server::request {

struct Header {
  std::string_view name;
  std::string value;

  Header(std::string_view name, std::string value)
      : name(name), value(std::move(value)) {}
};

using HeadersToPropagate = boost::container::small_vector<Header, 10>;

/// @brief Get a header from server::http::HttpRequest that is handled by the
/// current task hierarchy.
/// @return Header value or an empty string, if none such
const std::string& GetPropagatedHeader(std::string_view header_name);

/// @overload
const std::string& GetPropagatedHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name);

/// @brief Checks whether specified header exists in server::http::HttpRequest
/// that is handled by the current task hierarchy.
/// @return `true` if the header exists, `false` otherwise
bool HasPropagatedHeader(std::string_view header_name);

/// @overload
bool HasPropagatedHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name);

/// @brief Get a headers that is handled by the current task hierarchy.
boost::iterator_range<HeadersToPropagate::const_iterator>
GetPropagatedHeaders();

/// @brief Set a headers that is handled by the current task hierarchy.
void SetPropagatedHeaders(HeadersToPropagate headers);

/// @brief Get a query parameter from server::http::HttpRequest that is handled
/// by the current task hierarchy.
/// @return Parameter value or an empty string, if none such
const std::string& GetTaskInheritedQueryParameter(std::string_view name);

/// @brief Checks whether specified query parameter exists in
/// server::http::HttpRequest that is handled by the current task hierarchy.
/// @return `true` if the parameter exists, `false` otherwise
bool HasTaskInheritedQueryParameter(std::string_view name);

}  // namespace server::request

USERVER_NAMESPACE_END
