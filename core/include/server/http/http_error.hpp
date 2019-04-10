#pragma once

#include <stdexcept>
#include <string>

#include <server/handlers/exceptions.hpp>
#include "http_status.hpp"

namespace server {
namespace http {

#ifndef USERVER_NO_DEPRECATED_HTTP_ERRORS
// clang-format off
#define DEPRECATED_EXCEPTION(NEW_TYPE) \
  [[deprecated("Please use server::handlers::" #NEW_TYPE \
		  " instead. (see https://nda.ya.ru/3UYP5M)")]]
// clang-format on
#else
#define DEPRECATED_EXCEPTION(NEW_TYPE)
#endif

class DEPRECATED_EXCEPTION(CustomHandlerException) HttpException
    : public std::runtime_error {
 public:
  explicit HttpException(
      HttpStatus status, const std::string& internal_error_message,
      std::string external_error_body = std::string()) noexcept
      : std::runtime_error(internal_error_message),
        status_(status),
        external_error_body_(std::move(external_error_body)) {}

  HttpStatus GetStatus() const { return status_; }
  const std::string& GetExternalErrorBody() const {
    return external_error_body_;
  }

 private:
  const HttpStatus status_;
  const std::string external_error_body_;
};

class DEPRECATED_EXCEPTION(ClientError) BadRequest : public HttpException {
 public:
  explicit BadRequest(const std::string& internal_error_message = "Bad request",
                      std::string external_error_body = std::string())
      : HttpException(HttpStatus::kBadRequest, internal_error_message,
                      std::move(external_error_body)) {}
};

class DEPRECATED_EXCEPTION(Unauthorized) Unauthorized : public HttpException {
 public:
  explicit Unauthorized(
      const std::string& internal_error_message = "Unauthorized",
      std::string external_error_body = std::string()) noexcept
      : HttpException(server::http::HttpStatus::kUnauthorized,
                      internal_error_message, std::move(external_error_body)) {}
};

class DEPRECATED_EXCEPTION(InternalServerError) InternalServerError
    : public HttpException {
 public:
  explicit InternalServerError(
      const std::string& internal_error_message = "Internal server error",
      std::string external_error_body = std::string())
      : HttpException(HttpStatus::kInternalServerError, internal_error_message,
                      std::move(external_error_body)) {}
};

#undef DEPRECATED_EXCEPTION

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
