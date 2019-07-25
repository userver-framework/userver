#pragma once

#include <stdexcept>
#include <string>

#include <server/handlers/exceptions.hpp>
#include "http_status.hpp"

namespace server {
namespace http {

/**
 * Get http status code mapped to generic handler error code.
 * If there is no explicit mapping in the http_error.cpp, will return
 * HttpStatus::kBadRequest for code values less than
 * HandlerErrorCode::kServerSideError and will return
 * HttpStatus::kInternalServerError for the rest of codes.
 * @param
 * @return
 */
HttpStatus GetHttpStatus(handlers::HandlerErrorCode) noexcept;

}  // namespace http
}  // namespace server
