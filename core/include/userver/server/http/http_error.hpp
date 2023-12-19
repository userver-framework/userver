#pragma once

/// @file userver/server/http/http_error.hpp
/// @brief @copybrief server::http::GetHttpStatus

#include <stdexcept>
#include <string>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>

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

/// For server::http::CustomHandlerException, uses the provided HttpStatus.
/// For a generic server::handler::CustomHandlerException, converts its
/// server::handler::HandlerErrorCode to HttpStatus.
HttpStatus GetHttpStatus(
    const handlers::CustomHandlerException& exception) noexcept;

/// @brief An extension of server::handlers::CustomHandlerException that allows
/// to specify a custom HttpStatus.
///
/// For non-HTTP protocols, the status code will be derived from the attached
/// server::handlers::HandlerErrorCode.
class CustomHandlerException : public handlers::CustomHandlerException {
 public:
  /// @see server::handlers::CustomHandlerException for the description of how
  /// `args` that can augment error messages.
  /// @snippet server/handlers/exceptions_test.cpp  Sample construction HTTP
  template <typename... Args>
  CustomHandlerException(handlers::HandlerErrorCode error_code,
                         HttpStatus http_status, Args&&... args)
      : handlers::CustomHandlerException(error_code,
                                         std::forward<Args>(args)...),
        http_status_(http_status) {}

  HttpStatus GetHttpStatus() const { return http_status_; }

 private:
  HttpStatus http_status_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
