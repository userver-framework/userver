#pragma once

/// @file userver/server/http/http_status.hpp
/// @brief @copybrief server::http::HttpStatus

#include <string_view>

#include <fmt/core.h>

#include <userver/http/status_code.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief HTTP status codes
using HttpStatus = USERVER_NAMESPACE::http::StatusCode;

std::string_view HttpStatusString(HttpStatus status);

std::string ToString(HttpStatus status);
}  // namespace server::http

USERVER_NAMESPACE_END