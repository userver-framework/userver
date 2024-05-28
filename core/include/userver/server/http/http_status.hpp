#pragma once

/// @file userver/server/http/http_status.hpp
/// @brief @copybrief server::http::HttpStatus

#include <fmt/core.h>

#include <userver/http/status_code.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief HTTP status codes
using HttpStatus = USERVER_NAMESPACE::http::StatusCode;

[[deprecated("use StatusCodeString")]]
std::string_view HttpStatusString(HttpStatus status);

}  // namespace server::http

USERVER_NAMESPACE_END
