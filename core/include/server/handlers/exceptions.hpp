#pragma once

#include <stdexcept>
#include <string>

#include <utils/void_t.hpp>

namespace server {
namespace handlers {

/**
 * Enumeration that defines protocol-agnostic hander error condition codes.
 *
 * A handler for a specific protocol (e.g. http) should define mapping from
 * the HandlerErrorCode to protocol-specific error code.
 * @note When adding enumerators here, please add a description string to the
 * cpp file
 */
enum class HandlerErrorCode {
  kUnknownError,  //!< kUnknownError This value is to map possibly unknown codes
                  //!< to a description, shouldn't be used in client code
  kClientError,   //!< kInvalidRequest Invalid request data
  kRequestParseError,  //!< kRequestParseError
  kUnauthorized,  //!< kUnauthorized Client is not authorised to execute this
                  //!< handler
  kForbidden,     //!< kForbidden Requested action is forbidden
  kResourceNotFound,  //!< kResourceNoFound Requested resource doesn't exist
  kInvalidUsage,      //!< kInvalidUsage Invalid usage of the handler, e.g.
                      //!< unsupported HTTP method
  kNotAcceptable,     //!< kNotAcceptable The server cannot produce response,
                      //!< acceptable by the client
  kConfictState,  //!< kConfictState Request cannot be completed due to conflict
                  //!< resource state
  kPayloadTooLarge,  //!< kPayloadTooLarge The payload for the request exceeded
                     //!< handler's settings
  kTooManyRequests,  //!< kTooManyRequests Request limit exceeded
  // Client error codes should go before the server side error for them to be
  // mapped correctly to a protocol-specific error code
  // TODO More client-side error conditions here
  kServerSideError,  //!< kServerSideError An error occurred while processing
                     //!< the request
  kBadGateway,  //!< kBadGateway An error occured while passing the request to
                //!< another service

  kGatewayTimeout,  //!< kGatewayTimeout A timeout occured while passing the
                    //!< request to another service
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

struct InternalMessage {
  const std::string body;
};

struct ExternalBody {
  const std::string body;
};

namespace detail {

template <typename T, typename = utils::void_t<>>
struct HasInternalMessage : std::false_type {};
template <typename T>
constexpr bool kHasInternalMessage = HasInternalMessage<T>::value;

template <typename T>
struct HasInternalMessage<
    T, utils::void_t<decltype(std::declval<const T&>().GetInternalMessage())>>
    : std::true_type {};

template <typename T, typename = utils::void_t<>>
struct HasExternalBody : std::false_type {};
template <typename T>
constexpr bool kHasExternalBody = HasExternalBody<T>::value;

template <typename T>
struct HasExternalBody<
    T, utils::void_t<decltype(std::declval<const T&>().GetExternalBody())>>
    : std::true_type {};

template <typename T>
struct MessageExtractor {
  static_assert(kHasExternalBody<T>,
                "Please use your message builder to build external body for "
                "your error. See https://nda.ya.ru/3UYP5M for documentation");

  const T& builder;

  auto GetExternalBody() const { return builder.GetExternalBody(); }
  auto GetInternalMessage() const {
    if constexpr (kHasInternalMessage<T>) {
      return builder.GetInternalMessage();
    } else {
      return InternalMessage{};
    }
  }
};

}  // namespace detail

/**
 * Base class for handler exceptions.
 */
class CustomHandlerException : public std::runtime_error {
 public:
  // Type aliases for usage in descenant classes that are in other namespaces
  using HandlerErrorCode = handlers::HandlerErrorCode;
  using InternalMessage = handlers::InternalMessage;
  using ExternalBody = handlers::ExternalBody;

  constexpr static HandlerErrorCode kDefaultCode =
      HandlerErrorCode::kUnknownError;

 public:
  // Constructor for throwing with all arguments specified
  CustomHandlerException(ExternalBody external_body,
                         InternalMessage internal_message,
                         HandlerErrorCode code)
      : runtime_error(internal_message.body.empty() ? GetCodeDescription(code)
                                                    : internal_message.body),
        code_{code},
        external_body_{std::move(external_body.body)} {}

  template <typename MessageBuilder>
  CustomHandlerException(MessageBuilder&& builder, HandlerErrorCode code)
      : CustomHandlerException(
            detail::MessageExtractor<MessageBuilder>{builder}, code) {}

  HandlerErrorCode GetCode() const { return code_; }
  const std::string& GetExternalErrorBody() const { return external_body_; }

 private:
  template <typename MessageBuilder>
  CustomHandlerException(detail::MessageExtractor<MessageBuilder>&& builder,
                         HandlerErrorCode code)
      : CustomHandlerException(ExternalBody{builder.GetExternalBody()},
                               InternalMessage{builder.GetInternalMessage()},
                               code) {}
  const HandlerErrorCode code_;
  const std::string external_body_;
};

template <HandlerErrorCode Code>
class ExceptionWithCode : public CustomHandlerException {
 public:
  constexpr static HandlerErrorCode kDefaultCode = Code;
  using BaseType = ExceptionWithCode<kDefaultCode>;

  explicit ExceptionWithCode(ExternalBody external_body = {},
                             InternalMessage internal_message = {},
                             HandlerErrorCode code = kDefaultCode)
      : CustomHandlerException(std::move(external_body),
                               std::move(internal_message), code) {}
  explicit ExceptionWithCode(InternalMessage internal_message,
                             ExternalBody external_body = {},
                             HandlerErrorCode code = kDefaultCode)
      : CustomHandlerException(std::move(external_body),
                               std::move(internal_message), code) {}
  explicit ExceptionWithCode(HandlerErrorCode code,
                             ExternalBody external_body = {},
                             InternalMessage internal_message = {})
      : CustomHandlerException(std::move(external_body),
                               std::move(internal_message), code) {}
  ExceptionWithCode(HandlerErrorCode code, InternalMessage internal_message,
                    ExternalBody external_body = {})
      : CustomHandlerException(std::move(external_body),
                               std::move(internal_message), code) {}

  template <typename MessageBuilder>
  explicit ExceptionWithCode(MessageBuilder&& builder,
                             HandlerErrorCode code = kDefaultCode)
      : CustomHandlerException(builder, code) {}
  template <typename MessageBuilder>
  ExceptionWithCode(HandlerErrorCode code, MessageBuilder&& builder)
      : CustomHandlerException(builder, code) {}
};

/**
 * Base exception class for situations when request preconditions have failed.
 */
class ClientError : public ExceptionWithCode<HandlerErrorCode::kClientError> {
 public:
  using BaseType::BaseType;
};

/**
 * Exception class for situations when a request cannot be processed
 * due to parsing errors.
 */
class RequestParseError
    : public ExceptionWithCode<HandlerErrorCode::kRequestParseError> {
 public:
  using BaseType::BaseType;
};

/**
 * The handler requires authentication
 */
class Unauthorized : public ExceptionWithCode<HandlerErrorCode::kUnauthorized> {
 public:
  using BaseType::BaseType;
};

class ResourceNotFound
    : public ExceptionWithCode<HandlerErrorCode::kResourceNotFound> {
 public:
  using BaseType::BaseType;
};

/**
 * Base exception class for situations when an exception occurred while
 * processing the request.
 */
class InternalServerError
    : public ExceptionWithCode<HandlerErrorCode::kServerSideError> {
 public:
  using BaseType::BaseType;
};

}  // namespace handlers
}  // namespace server
