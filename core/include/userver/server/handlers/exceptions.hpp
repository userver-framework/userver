#pragma once

/// @file userver/server/handlers/exceptions.hpp
/// @brief Helpers for throwing exceptions

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <userver/formats/json.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/meta_light.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// Enumeration that defines protocol-agnostic handler error condition codes,
/// used by server::handlers::CustomHandlerException.
///
/// Specific error formats can derive various defaults from the this code, e.g.
/// HTTP status code, JSON service error code, and default error message texts.
///
/// For example, to provide an HTTP specific error code that is not presented
/// in this enum the HTTP code should be provided via
/// server::http::CustomHandlerException construction with the required
/// server::http::HttpStatus.
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

  // Client error codes are declared before the server side error to be
  // mapped correctly to a protocol-specific error code!

  kServerSideError,  //!< kServerSideError An error occurred while processing
                     //!< the request
  kBadGateway,  //!< kBadGateway An error occurred while passing the request to
                //!< another service

  kGatewayTimeout,  //!< kGatewayTimeout A timeout occurred while passing the
                    //!< request to another service
  kUnsupportedMediaType,  //!< kUnsupportedMediaType Content-Encoding or
                          //!< Content-Type is not supported

  // Server error codes are declared ater the client side error to be
  // mapped correctly to a protocol-specific error code!
};
// When adding enumerators here ^, please also add mappings to the
// implementation of:
// - server::handler::GetCodeDescription,
// - server::handler::GetFallbackServiceCode,
// - server::http::GetHttpStatus.

/// Hasher class for HandlerErrorCode
struct HandlerErrorCodeHash {
  std::size_t operator()(HandlerErrorCode c) const {
    return static_cast<std::size_t>(c);
  }
};

using Headers = std::unordered_map<std::string, std::string,
                                   utils::StrIcaseHash, utils::StrIcaseEqual>;

std::string_view GetCodeDescription(HandlerErrorCode);

std::string_view GetFallbackServiceCode(HandlerErrorCode);

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

template <typename T>
using IsExternalBodyFormatted = std::bool_constant<T::kIsExternalBodyFormatted>;

template <typename T>
using HasServiceCode = decltype(std::declval<const T&>().GetServiceCode());

template <typename T>
using HasInternalMessage =
    decltype(std::declval<const T&>().GetInternalMessage());

template <typename T>
inline constexpr bool kHasInternalMessage =
    meta::kIsDetected<HasInternalMessage, T>;

template <typename T>
using HasExternalBody = decltype(std::declval<const T&>().GetExternalBody());

template <typename T>
inline constexpr bool kHasExternalBody = meta::kIsDetected<HasExternalBody, T>;

template <typename T>
inline constexpr bool kIsMessageBuilder = kHasExternalBody<T>;

template <typename T>
struct MessageExtractor {
  static_assert(meta::kIsDetected<HasExternalBody, T>,
                "Please use your message builder to build external body for "
                "your error. See server::handlers::CustomHandlerException "
                "for more info");

  const T& builder;

  constexpr bool IsExternalBodyFormatted() const {
    return meta::DetectedOr<std::false_type, impl::IsExternalBodyFormatted,
                            T>::value;
  }

  std::string GetServiceCode() const {
    if constexpr (meta::kIsDetected<HasServiceCode, T>) {
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

/// @brief The generic base class for handler exceptions. Thrown exceptions
/// should typically derive from ExceptionWithCode instead.
class CustomHandlerException : public std::runtime_error {
 public:
  // Type aliases for usage in descendent classes that are in other namespaces
  using HandlerErrorCode = handlers::HandlerErrorCode;
  using ServiceErrorCode = handlers::ServiceErrorCode;
  using InternalMessage = handlers::InternalMessage;
  using ExternalBody = handlers::ExternalBody;
  using ExtraHeaders = handlers::ExtraHeaders;

  /// @brief Construct manually from a set of (mostly optional) arguments, which
  /// describe the error details.
  ///
  /// ## Allowed arguments
  ///
  /// - HandlerErrorCode - defaults for other fields are derived from it
  /// - ServiceErrorCode - used e.g. in JSON error format
  /// - http::HttpStatus
  /// - InternalMessage - for logs
  /// - ExternalBody
  /// - ExtraHeaders
  /// - formats::json::Value - details for JSON error format
  /// - a message builder, see below
  ///
  /// Example:
  /// @snippet server/handlers/exceptions_test.cpp  Sample direct construction
  ///
  /// ## Message builders
  ///
  /// A message builder is a class that satisfies the following requirements:
  /// - provides a `GetExternalBody() const` function to form an external body
  /// - has an optional `kIsExternalBodyFormatted` set to true
  ///   to forbid changing the external body
  /// - has an optional `GetServiceCode() const` function to return machine
  ///   readable error code
  /// - has an optional `GetInternalMessage() const` function to form an message
  ///   for logging an error
  ///
  /// Some message builder data can be overridden by explicitly passed args, if
  /// these args go *after* the message builder.
  ///
  /// Example:
  /// @snippet server/handlers/exceptions_test.cpp  Sample custom error builder
  template <typename... Args>
  CustomHandlerException(HandlerErrorCode handler_code, Args&&... args)
      : CustomHandlerException(impl::CustomHandlerExceptionData{
            handler_code, std::forward<Args>(args)...}) {}

  /// @overload
  explicit CustomHandlerException(HandlerErrorCode handler_code)
      : CustomHandlerException(impl::CustomHandlerExceptionData{handler_code}) {
  }

  /// @deprecated Use the variadic constructor above instead.
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

  /// @deprecated Use the variadic constructor above instead.
  template <
      typename MessageBuilder,
      typename = std::enable_if_t<impl::kIsMessageBuilder<MessageBuilder>>>
  CustomHandlerException(MessageBuilder&& builder,
                         HandlerErrorCode handler_code)
      : CustomHandlerException(impl::CustomHandlerExceptionData{
            std::forward<MessageBuilder>(builder), handler_code}) {}

  /// @cond
  explicit CustomHandlerException(impl::CustomHandlerExceptionData&& data)
      : runtime_error(data.internal_message.empty()
                          ? std::string{GetCodeDescription(data.handler_code)}
                          : data.internal_message),
        data_(std::move(data)) {
    UASSERT_MSG(data_.details.IsNull() || data_.details.IsObject(),
                "The details JSON value must be either null or an object");
  }
  /// @endcond

  HandlerErrorCode GetCode() const { return data_.handler_code; }

  const std::string& GetServiceCode() const { return data_.service_code; }

  bool IsExternalErrorBodyFormatted() const {
    return data_.is_external_body_formatted;
  }

  const std::string& GetExternalErrorBody() const {
    return data_.external_body;
  }

  const formats::json::Value& GetDetails() const { return data_.details; }

  const Headers& GetExtraHeaders() const { return data_.headers; }

 private:
  impl::CustomHandlerExceptionData data_;
};

/// @brief Base class for handler exceptions. For common HandlerErrorCode values
/// you can use more specific exception classes below. For less common
/// HandlerErrorCode values, this class can be used directly.
template <HandlerErrorCode Code>
class ExceptionWithCode : public CustomHandlerException {
 public:
  constexpr static HandlerErrorCode kDefaultCode = Code;
  using BaseType = ExceptionWithCode<kDefaultCode>;

  ExceptionWithCode(const ExceptionWithCode<Code>&) = default;
  ExceptionWithCode(ExceptionWithCode<Code>&&) noexcept = default;

  /// @see CustomHandlerException::CustomHandlerException for allowed args
  template <typename... Args>
  explicit ExceptionWithCode(Args&&... args)
      : CustomHandlerException(kDefaultCode, std::forward<Args>(args)...) {}
};

/// Exception class for situations when request preconditions have failed.
/// Corresponds to HTTP code 400.
class ClientError : public ExceptionWithCode<HandlerErrorCode::kClientError> {
 public:
  using BaseType::BaseType;
};

/// Exception class for situations when a request cannot be processed
/// due to parsing errors. Corresponds to HTTP code 400.
class RequestParseError
    : public ExceptionWithCode<HandlerErrorCode::kRequestParseError> {
 public:
  using BaseType::BaseType;
};

/// Exception class for situations when a request does not have valid
/// authentication credentials. Corresponds to HTTP code 401.
class Unauthorized : public ExceptionWithCode<HandlerErrorCode::kUnauthorized> {
 public:
  using BaseType::BaseType;
};

/// Exception class for situations when some requested entity could not be
/// accessed, because it does not exist. Corresponds to HTTP code 404.
class ResourceNotFound
    : public ExceptionWithCode<HandlerErrorCode::kResourceNotFound> {
 public:
  using BaseType::BaseType;
};

/// Exception class for situations when a conflict happens, e.g. the client
/// attempts to create an entity that already exists. Corresponds to
/// HTTP code 409.
class ConflictError : public server::handlers::ExceptionWithCode<
                          server::handlers::HandlerErrorCode::kConflictState> {
 public:
  using BaseType::BaseType;
};

/// Exception class for situations when an exception occurred while processing
/// the request. Corresponds to HTTP code 500.
class InternalServerError
    : public ExceptionWithCode<HandlerErrorCode::kServerSideError> {
 public:
  using BaseType::BaseType;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
