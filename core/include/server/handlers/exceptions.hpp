#pragma once

#include <stdexcept>
#include <string>

namespace server {
namespace handlers {

/**
 * Enumeration that defines protocol-agnostic hander error condition codes.
 *
 * A handler for a specific protocol (e.g. http) should define mapping from
 * the HandlerErrorCode to protocol-specific error code.
 */
// Note: When adding enumerators here, please add a description string to the
// cpp file
enum class HandlerErrorCode {
  kUnknownError,  //!< kUnknownError This value is to map possibly unknown codes
                  //!< to a description, shouldn't be used in client code
  kClientError,   //!< kInvalidRequest Invalid requests data
  kUnauthorized,  //!< kUnauthorized Client is not authorised to execute this
                  //!< handler
  kRequestParseError,  //!< kRequestParseError
  // Client error codes should go before the server side error for them to be
  // mapped correctly to a protocol-specific error code
  // TODO More client-side error conditions here
  kServerSideError,  //!< kServerSideError An error occurred while processing
                     //!< the request
  // TODO More server-side error conditions
};

/**
 * Hasher class for HanderErrorCode
 */
struct HandlerErrorCodeHash {
  std::size_t operator()(HandlerErrorCode c) const {
    return static_cast<std::size_t>(c);
  }
};

const char* GetCodeDescription(HandlerErrorCode) noexcept;

/**
 * Base class for handler exceptions.
 */
class CustomHandlerException : public std::runtime_error {
 public:
  CustomHandlerException(
      std::string external_body, HandlerErrorCode code,
      const std::string& internal_message = std::string{}) noexcept
      : runtime_error(internal_message.empty() ? GetCodeDescription(code)
                                               : internal_message),
        code_{code},
        external_body_{std::move(external_body)} {}

  HandlerErrorCode GetCode() const { return code_; }
  const std::string& GetExternalErrorBody() const { return external_body_; }

 private:
  const HandlerErrorCode code_;
  const std::string external_body_;
};

/**
 * Base exception class for situations when request preconditions have failed.
 */
class ClientError : public CustomHandlerException {
 public:
  explicit ClientError(std::string external_body = std::string{},
                       HandlerErrorCode code = HandlerErrorCode::kClientError,
                       const std::string& internal_message = std::string{})
      : CustomHandlerException(std::move(external_body), code,
                               internal_message) {}
  explicit ClientError(HandlerErrorCode code,
                       std::string external_body = std::string{},
                       const std::string& internal_message = std::string{})
      : CustomHandlerException(std::move(external_body), code,
                               internal_message) {}
};

/**
 * Base exception class for situations when an exception occurred while
 * processing the request.
 */
class InternalServerError : public CustomHandlerException {
 public:
  explicit InternalServerError(
      std::string external_body = std::string{},
      HandlerErrorCode code = HandlerErrorCode::kServerSideError,
      const std::string& internal_message = std::string{})
      : CustomHandlerException(std::move(external_body), code,
                               internal_message) {}
  explicit InternalServerError(
      HandlerErrorCode code, std::string external_body = std::string{},
      const std::string& internal_message = std::string{})
      : CustomHandlerException(std::move(external_body), code,
                               internal_message) {}
};

}  // namespace handlers
}  // namespace server
