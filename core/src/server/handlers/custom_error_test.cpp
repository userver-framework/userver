#include <gtest/gtest.h>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/handler_base.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::test {

TEST(CustomHandlerException, MappedCodes) {
  EXPECT_EQ(http::HttpStatus::kBadRequest,
            http::GetHttpStatus(HandlerErrorCode::kClientError));
  EXPECT_EQ(http::HttpStatus::kBadRequest,
            http::GetHttpStatus(HandlerErrorCode::kRequestParseError));
  EXPECT_EQ(http::HttpStatus::kUnauthorized,
            http::GetHttpStatus(HandlerErrorCode::kUnauthorized));
  EXPECT_EQ(http::HttpStatus::kMethodNotAllowed,
            http::GetHttpStatus(HandlerErrorCode::kInvalidUsage));
  EXPECT_EQ(http::HttpStatus::kNotFound,
            http::GetHttpStatus(HandlerErrorCode::kResourceNotFound));
  EXPECT_EQ(http::HttpStatus::kConflict,
            http::GetHttpStatus(HandlerErrorCode::kConflictState));
  EXPECT_EQ(http::HttpStatus::kPayloadTooLarge,
            http::GetHttpStatus(HandlerErrorCode::kPayloadTooLarge));
  EXPECT_EQ(http::HttpStatus::kTooManyRequests,
            http::GetHttpStatus(HandlerErrorCode::kTooManyRequests));

  EXPECT_EQ(http::HttpStatus::kInternalServerError,
            http::GetHttpStatus(HandlerErrorCode::kServerSideError));
  EXPECT_EQ(http::HttpStatus::kBadGateway,
            http::GetHttpStatus(HandlerErrorCode::kBadGateway));
}

struct CustomMessage {
  std::string message;

  const std::string& GetInternalMessage() const { return message; }
  const std::string& GetExternalBody() const { return message; }
};

struct CustomMessageWithServiceCode {
  std::string service_code;
  std::string message;

  const std::string& GetServiceCode() const { return service_code; }
  const std::string& GetInternalMessage() const { return message; }
  const std::string& GetExternalBody() const { return message; }
};

struct CustomFormattedMessage {
  static constexpr bool kIsExternalBodyFormatted = true;

  std::string message;

  const std::string& GetInternalMessage() const { return message; }
  const std::string& GetExternalBody() const { return message; }
};

template <typename Error>
class CustomHandlerExceptionTest : public ::testing::Test {
 public:
  using ErrorType = Error;
  static constexpr auto kDefaultCode = ErrorType::kDefaultCode;
};

using ErrorTypes = ::testing::Types<ClientError, Unauthorized, ResourceNotFound,
                                    ConflictError, InternalServerError>;
TYPED_TEST_SUITE(CustomHandlerExceptionTest, ErrorTypes);

TYPED_TEST(CustomHandlerExceptionTest, AllDefault) {
  using ErrorType = typename TestFixture::ErrorType;
  constexpr auto kDefaultCode = TestFixture::kDefaultCode;
  UEXPECT_THROW(throw ErrorType(), ErrorType);
  try {
    throw ErrorType();
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_EQ(GetCodeDescription(kDefaultCode), e.what());
    EXPECT_TRUE(e.GetServiceCode().empty());
    EXPECT_TRUE(e.GetExternalErrorBody().empty());
    EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
  }
}

TYPED_TEST(CustomHandlerExceptionTest, OverrideMessage) {
  using ErrorType = typename TestFixture::ErrorType;
  constexpr auto kDefaultCode = TestFixture::kDefaultCode;
  try {
    throw ErrorType(InternalMessage{"Something went wrong"},
                    ExternalBody{"Tell everyone about it"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ("Something went wrong", e.what());
    EXPECT_TRUE(e.GetServiceCode().empty());
    EXPECT_EQ("Tell everyone about it", e.GetExternalErrorBody());
    EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
  }
}

TYPED_TEST(CustomHandlerExceptionTest, ServiceCode) {
  using ErrorType = typename TestFixture::ErrorType;
  constexpr auto kDefaultCode = TestFixture::kDefaultCode;
  try {
    throw ErrorType(ServiceErrorCode{"custom_code"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_EQ(GetCodeDescription(kDefaultCode), e.what());
    EXPECT_EQ("custom_code", e.GetServiceCode());
    EXPECT_TRUE(e.GetExternalErrorBody().empty());
    EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
  }
  try {
    throw ErrorType(
        CustomMessageWithServiceCode{"custom_code", "custom_message"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ("custom_message", e.what());
    EXPECT_EQ("custom_code", e.GetServiceCode());
    EXPECT_EQ("custom_message", e.GetExternalErrorBody());
    EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
  }
}

TYPED_TEST(CustomHandlerExceptionTest, MessageBuilder) {
  using ErrorType = typename TestFixture::ErrorType;
  constexpr auto kDefaultCode = TestFixture::kDefaultCode;
  try {
    throw ErrorType(CustomMessage{"Something went wrong"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ("Something went wrong", e.what());
    EXPECT_TRUE(e.GetServiceCode().empty());
    EXPECT_EQ("Something went wrong", e.GetExternalErrorBody());
    EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
  }
}

TYPED_TEST(CustomHandlerExceptionTest, CustomCode) {
  using ErrorType = typename TestFixture::ErrorType;
  for (auto code :
       {HandlerErrorCode::kUnknownError, HandlerErrorCode::kClientError,
        HandlerErrorCode::kUnauthorized, HandlerErrorCode::kInvalidUsage,
        HandlerErrorCode::kRequestParseError,
        HandlerErrorCode::kResourceNotFound, HandlerErrorCode::kTooManyRequests,
        HandlerErrorCode::kServerSideError, HandlerErrorCode::kBadGateway,
        HandlerErrorCode::kConflictState}) {
    try {
      // Custom code, messages defaulted
      throw ErrorType(code);
    } catch (const ErrorType& e) {
      EXPECT_EQ(code, e.GetCode());
      EXPECT_EQ(GetCodeDescription(code), e.what());
      EXPECT_TRUE(e.GetServiceCode().empty());
      EXPECT_TRUE(e.GetExternalErrorBody().empty());
      EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
    }
    try {
      // Custom code, custom message
      throw ErrorType(code, CustomMessage{"Something went wrong"});
    } catch (const ErrorType& e) {
      EXPECT_EQ(code, e.GetCode());
      EXPECT_STREQ("Something went wrong", e.what());
      EXPECT_TRUE(e.GetServiceCode().empty());
      EXPECT_EQ("Something went wrong", e.GetExternalErrorBody());
      EXPECT_FALSE(e.IsExternalErrorBodyFormatted());
    }
  }
}

TYPED_TEST(CustomHandlerExceptionTest, DoubleFormat) {
  using ErrorType = typename TestFixture::ErrorType;
  constexpr auto kDefaultCode = TestFixture::kDefaultCode;
  try {
    throw ErrorType(CustomFormattedMessage{"test"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ("test", e.what());
    EXPECT_TRUE(e.GetServiceCode().empty());
    EXPECT_EQ("test", e.GetExternalErrorBody());
    EXPECT_TRUE(e.IsExternalErrorBodyFormatted());
  }
}

}  // namespace server::handlers::test

USERVER_NAMESPACE_END
