#pragma once

/// @file userver/server/request/task_inherited_request.hpp
/// @brief Functions that provide access to HttpRequest stored in
/// TaskInheritedVariable.

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace http::headers {
class PredefinedHeader;
}

namespace server::request {

/// @brief Get a header from server::http::HttpRequest that is handled by the
/// current task hierarchy.
/// @return Header value or an empty string, if none such
const std::string& GetTaskInheritedHeader(std::string_view header_name);

/// @overload
const std::string& GetTaskInheritedHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name);

/// @brief Checks whether specified header exists in server::http::HttpRequest
/// that is handled by the current task hierarchy.
/// @return `true` if the header exists, `false` otherwise
bool HasTaskInheritedHeader(std::string_view header_name);

/// @overload
bool HasTaskInheritedHeader(
    const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name);

}  // namespace server::request

USERVER_NAMESPACE_END
