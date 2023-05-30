#pragma once

/// @file userver/server/handlers/exceptions.hpp
/// @brief Helpers for throwing exceptions

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <userver/formats/json.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/**
 * Enumeration that defines protocol-agnostic handler error condition codes,
 * used by server::handlers::CustomHandlerException.
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
  kConflictState,     //!< kConflictState Request cannot be completed due to
                      //!< conflict resource state
  kPayloadTooLarge,   //!< kPayloadTooLarge The payload for the request exceeded
                      //!< handler's settings
  kTooManyRequests,   //!< kTooManyRequests Request limit exceeded
  // Client error codes should go before the server side error for them to be
  // mapped correctly to a protocol-specific error code
  // TODO More client-side error conditions here
  kServerSideError,  //!< kServerSideError An error occurred while processing
                     //!< the request
  kBadGateway,  //!< kBadGateway An error occurred while passing the request to
                //!< another service

  kGatewayTimeout,  //!< kGatewayTimeout A timeout occurred while passing the
                    //!< request to another service
  kUnsupportedMediaType,  //!< kUnsupportedMediaType Content-Encoding or
                          //!< Content-Type is not supported
  // TODO More server-side error conditions
};

/**
 * Hasher class for HandlerErrorCode
 */
struct HandlerErrorCodeHash {
  std::size_t operator()(HandlerErrorCode c) const {
    return static_cast<std::size_t>(c);
  }
};

using Headers = std::unordered_map<std::string, std::string,
                                   utils::StrIcaseHash, utils::StrIcaseEqual>;

std::string GetCodeDescription(HandlerErrorCode);

std::string GetFallbackServiceCode(HandlerErrorCode);

struct ServiceErrorCode {
  std::string body;
};

struct InternalMessage {
  std::string body;
};

struct ExternalBody {
  std::string body;
};

struct ExtraHeaders {
  Headers headers;
};

namespace impl {

template <typename T, typename = utils::void_t<>>
struct IsExternalBodyFormatted : std::false_type {};
template <typename T>
inline constexpr bool kIsExternalBodyFormatted =
    IsExternalBodyFormatted<T>::value;

template <typename T>
struct IsExternalBodyFormatted<
    T, utils::void_t<decltype(T::kIsExternalBodyFormatted)>>
    : std::integral_constant<bool, T::kIsExternalBodyFormatted> {};

template <typename T, typename = utils::void_t<>>
struct HasServiceCode : std::false_type {};
template <typename T>
inline constexpr bool kHasServiceCode = HasServiceCode<T>::value;

template <typename T>
struct HasServiceCode<
    T, utils::void_t<decltype(std::declval<const T&>().GetServiceCode())>>
    : std::true_type {};

template <typename T, typename = utils::void_t<>>
struct HasInternalMessage : std::false_type {};
template <typename T>
inline constexpr bool kHasInternalMessage = HasInternalMessage<T>::value;

template <typename T>
struct HasInternalMessage<
    T, utils::void_t<decltype(std::declval<const T&>().GetInternalMessage())>>
    : std::true_type {};

template <typename T, typename = utils::void_t<>>
struct HasExternalBody : std::false_type {};
template <typename T>
inline constexpr bool kHasExternalBody = HasExternalBody<T>::value;

template <typename T>
struct HasExternalBody<
    T, utils::void_t<decltype(std::declval<const T&>().GetExternalBody())>>
    : std::true_type {};

template <typename T>
struct MessageExtractor {
  static_assert(kHasExternalBody<T>,
                "Please use your message builder to build external body for "
                "your error. See server::handlers::CustomHandlerException "
                "for more info");

  const T& builder;

  constexpr bool IsExternalBodyFormatted() const {
    return kIsExternalBodyFormatted<T>;
  }

  std::string GetServiceCode() const {
    if constexpr (kHasServiceCode<T>) {
      return builder.GetServiceCode();
    } else {
      return std::string{};
    }
  }

  std::string GetExternalBody() const { return builder.GetExternalBody(); }

  std::string GetInternalMessage() const {
    if constexpr (kHasInternalMessage<T>) {
      return builder.GetInternalMessage();
    } else {
      return std::string{};
    }
  }
};

struct CustomHandlerExceptionData final {
  CustomHandlerExceptionData() = default;
  CustomHandlerExceptionData(const CustomHandlerExceptionData&) = default;
  CustomHandlerExceptionData(CustomHandlerExceptionData&&) noexcept = default;

  template <typename... Args>
  explicit CustomHandlerExceptionData(Args&&... args) {
    (Apply(std::forward<Args>(args)), ...);
  }

  bool is_external_body_formatted{false};
  HandlerErrorCode handler_code{HandlerErrorCode::kUnknownError};
  std::string service_code;
  std::string internal_message;
  std::string external_body;
  Headers headers;
  formats::json::Value details;

 private:
  void Apply(HandlerErrorCode handler_code_) { handler_code = handler_code_; }

  void Apply(ServiceErrorCode service_code_) {
    service_code = std::move(service_code_.body);
  }

  void Apply(InternalMessage internal_message_) {
    internal_message = std::move(internal_message_.body);
  }

  void Apply(ExternalBody external_body_) {
    external_body = std::move(external_body_.body);
  }

  void Apply(ExtraHeaders headers_) { headers = std::move(headers_.headers); }

  void Apply(formats::json::Value details_) { details = std::move(details_); }

  template <typename MessageBuilder>
  void Apply(MessageBuilder&& builder) {
    impl::MessageExtractor<MessageBuilder> extractor{builder};
    is_external_body_formatted = extractor.IsExternalBodyFormatted();
    service_code = extractor.GetServiceCode();
    external_body = extractor.GetExternalBody();
    internal_message = extractor.GetInternalMessage();
  }
};

}  // namespace impl

// clang-format off

/**
 * @brief Base class for handler exceptions.
 *
 * For constructing the body of an exception a special message builder type could be
 * used. Message builder should satisfy the following requirements:
 * - has an optional `kIsExternalBodyFormatted` set to true to forbid changing the external body
 * - has an optional `GetServiceCode()` function to return machine readable error code
 * - provides a `GetExternalBody() const` function to form an external body
 * - has an optional `GetInternalMessage() const` function to form an message for logging an error
 *
 * Example:
 * @snippet server/handlers/exceptions_test.cpp  Sample custom error builder
 */

// clang-format on
class CustomHandlerException : public std::runtime_error {
 public:
  // Type aliases for usage in descendent classes that are in other namespaces
  using HandlerErrorCode = handlers::HandlerErrorCode;
  using ServiceErrorCode = handlers::ServiceErrorCode;
  using InternalMessage = handlers::InternalMessage;
  using ExternalBody = handlers::ExternalBody;
  using ExtraHeaders = handlers::ExtraHeaders;

  constexpr static HandlerErrorCode kDefaultCode =
      HandlerErrorCode::kUnknownError;

  CustomHandlerException(impl::CustomHandlerExceptionData data)
      : runtime_error(data.internal_message.empty()
                          ? GetCodeDescription(data.handler_code)
                          : data.internal_message),
        data_(std::move(data)) {
    UASSERT_MSG(data_.details.IsNull() || data_.details.IsObject(),
                "The details JSON value must be either null or an object");
  }

  // Constructor for throwing with all arguments specified
  CustomHandlerException(ServiceErrorCode service_code,
                         ExternalBody external_body,
                         InternalMessage internal_message,
                         HandlerErrorCode handler_code,
                         ExtraHeaders headers = {},
                         formats::json::Value details = {})
      : CustomHandlerException(impl::CustomHandlerExceptionData{
            std::move(service_code), std::move(external_body),
            std::move(internal_message), handler_code, std::move(headers),
            std::move(details)}) {}

  template <typename MessageBuilder>
  CustomHandlerException(MessageBuilder&& builder,
                         HandlerErrorCode handler_code)
      : CustomHandlerException(impl::CustomHandlerExceptionData{
            std::forward<MessageBuilder>(builder), handler_code}) {}

  HandlerErrorCode GetCode() const { return data_.handler_code; }

  const std::string& GetServiceCode() const { return data_.service_code; }

  bool IsExternalErrorBodyFormatted() const {
    return data_.is_external_body_formatted;
  }

  const std::string& GetExternalErrorBody() const {
    return data_.external_body;
  }

  const formats::json::Value& GetDetails() const { return data_.details; };

  const Headers& GetExtraHeaders() const { return data_.headers; }

 private:
  impl::CustomHandlerExceptionData data_;
};

template <HandlerErrorCode Code>
class ExceptionWithCode : public CustomHandlerException {
 public:
  constexpr static HandlerErrorCode kDefaultCode = Code;
  using BaseType = ExceptionWithCode<kDefaultCode>;

  ExceptionWithCode(const ExceptionWithCode<Code>&) = default;
  ExceptionWithCode(ExceptionWithCode<Code>&&) noexcept = default;

  template <typename... Args>
  explicit ExceptionWithCode(Args&&... args)
      : CustomHandlerException(impl::CustomHandlerExceptionData(
            kDefaultCode, std::forward<Args>(args)...)) {}
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
 * Base exception class for situations when conflict happens.
 */
class ConflictError : public server::handlers::ExceptionWithCode<
                          server::handlers::HandlerErrorCode::kConflictState> {
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

}  // namespace server::handlers

USERVER_NAMESPACE_END
