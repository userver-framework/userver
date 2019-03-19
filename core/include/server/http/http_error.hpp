#pragma once

#include <stdexcept>
#include <string>

#include <server/handlers/exceptions.hpp>
#include "http_status.hpp"

namespace server {
namespace http {

class HttpException : public std::runtime_error {
 public:
  explicit HttpException(
      HttpStatus status, std::string internal_error_message,
      std::string external_error_body = std::string()) noexcept
      : std::runtime_error(std::move(internal_error_message)),
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

class BadRequest : public HttpException {
 public:
  explicit BadRequest(std::string internal_error_message = "Bad request",
                      std::string external_error_body = std::string())
      : HttpException(HttpStatus::kBadRequest,
                      std::move(internal_error_message),
                      std::move(external_error_body)) {}
};

class Unauthorized : public HttpException {
 public:
  explicit Unauthorized(
      std::string internal_error_message = "Unauthorized",
      std::string external_error_body = std::string()) noexcept
      : HttpException(server::http::HttpStatus::kUnauthorized,
                      std::move(internal_error_message),
                      std::move(external_error_body)) {}
};

class InternalServerError : public HttpException {
 public:
  explicit InternalServerError(
      std::string internal_error_message = "Internal server error",
      std::string external_error_body = std::string())
      : HttpException(HttpStatus::kInternalServerError,
                      std::move(internal_error_message),
                      std::move(external_error_body)) {}
};

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
