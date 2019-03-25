#include <gtest/gtest.h>

#include <server/handlers/exceptions.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/http/http_error.hpp>

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
            http::GetHttpStatus(HandlerErrorCode::kConfictState));
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

template <typename Error>
class CustomHandlerExceptionTest : public ::testing::Test {
 public:
  using ErrorType = Error;
  static constexpr auto kDefaultCode = ErrorType::kDefaultCode;
};

using ErrorTypes = ::testing::Types<ClientError, Unauthorized, ResourceNotFound,
                                    InternalServerError>;
TYPED_TEST_CASE(CustomHandlerExceptionTest, ErrorTypes);

TYPED_TEST(CustomHandlerExceptionTest, CorrectContents) {
  using ErrorType = typename TestFixture::ErrorType;
  constexpr auto kDefaultCode = TestFixture::kDefaultCode;
  EXPECT_THROW(throw ErrorType(), ErrorType);
  try {
    // All parameters defaulted
    throw ErrorType();
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ(GetCodeDescription(kDefaultCode), e.what());
    EXPECT_TRUE(e.GetExternalErrorBody().empty());
  }
  try {
    // Code defaulted, custom messages
    throw ErrorType(InternalMessage{"Something went wrong"},
                    ExternalBody{"Tell everyone about it"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ("Something went wrong", e.what());
    EXPECT_EQ("Tell everyone about it", e.GetExternalErrorBody());
  }
  try {
    // Code defaulted, custom message builder
    throw ErrorType(CustomMessage{"Something went wrong"});
  } catch (const ErrorType& e) {
    EXPECT_EQ(kDefaultCode, e.GetCode());
    EXPECT_STREQ("Something went wrong", e.what());
    EXPECT_EQ("Something went wrong", e.GetExternalErrorBody());
  }
  for (auto code :
       {HandlerErrorCode::kUnknownError, HandlerErrorCode::kClientError,
        HandlerErrorCode::kUnauthorized, HandlerErrorCode::kInvalidUsage,
        HandlerErrorCode::kRequestParseError,
        HandlerErrorCode::kResourceNotFound, HandlerErrorCode::kConfictState,
        HandlerErrorCode::kTooManyRequests, HandlerErrorCode::kServerSideError,
        HandlerErrorCode::kBadGateway}) {
    try {
      // Custom code, messages defaulted
      throw ErrorType(code);
    } catch (const ErrorType& e) {
      EXPECT_EQ(code, e.GetCode());
      EXPECT_STREQ(GetCodeDescription(code), e.what());
      EXPECT_TRUE(e.GetExternalErrorBody().empty());
    }
    try {
      // Custom code, custom message
      throw ErrorType(code, CustomMessage{"Something went wrong"});
    } catch (const ErrorType& e) {
      EXPECT_EQ(code, e.GetCode());
      EXPECT_STREQ("Something went wrong", e.what());
      EXPECT_EQ("Something went wrong", e.GetExternalErrorBody());
    }
  }
}

}  // namespace server::handlers::test
