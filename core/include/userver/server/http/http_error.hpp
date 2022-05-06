#pragma once

/// @file userver/server/http/http_error.hpp
/// @brief @copybrief server::http::GetHttpStatus

#include <stdexcept>
#include <string>

#include <userver/server/handlers/exceptions.hpp>
#include "http_status.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

/**
 * @brief Get http status code mapped to generic handler error code.
 *
 * If there is no explicit mapping in the http_error.cpp, will return
 * HttpStatus::kBadRequest for code values less than
 * HandlerErrorCode::kServerSideError and will return
 * HttpStatus::kInternalServerError for the rest of codes.
 */
HttpStatus GetHttpStatus(handlers::HandlerErrorCode) noexcept;

}  // namespace server::http

USERVER_NAMESPACE_END
