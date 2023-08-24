#include <userver/server/handlers/exceptions.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/server/http/http_error.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

/// [Sample custom error builder]
class CustomErrorBuilder {
 public:
  static constexpr bool kIsExternalBodyFormatted = true;

  CustomErrorBuilder(std::string status, std::string msg) {
    formats::json::ValueBuilder error;
    error["code"] = std::move(status);
    error["message"] = std::move(msg);
    json_error_body = formats::json::ToString(error.ExtractValue());
  }

  std::string GetExternalBody() const { return json_error_body; }

 private:
  std::string json_error_body;
};

TEST(CustomHandlerException, BuilderSample) {
  auto exc = server::handlers::ClientError(
      CustomErrorBuilder{"Bad Request", "Something went wrong"});

  EXPECT_EQ(exc.GetCode(), server::handlers::HandlerErrorCode::kClientError);
  EXPECT_EQ(exc.what(), std::string{"Client error"}) << "Log message";

  auto json = formats::json::FromString(
      R"({"code":"Bad Request","message":"Something went wrong"})");
  EXPECT_EQ(formats::json::FromString(exc.GetExternalErrorBody()), json);
}
/// [Sample custom error builder]

/// [Sample direct construction]
TEST(CustomHandlerException, DirectSample) {
  auto exc = server::handlers::ClientError(
      server::handlers::InternalMessage{"Spam detected, criterion: too many "
                                        "repetitions (42, we only allow 10)"},
      server::handlers::ExternalBody{"Failed to post: spam detected"});

  EXPECT_EQ(exc.GetCode(), server::handlers::HandlerErrorCode::kClientError);
  EXPECT_EQ(server::http::GetHttpStatus(exc),
            server::http::HttpStatus::kBadRequest);
  EXPECT_THAT(exc.GetExternalErrorBody(),
              testing::HasSubstr("Failed to post: spam detected"));
  EXPECT_THAT(exc.what(), testing::HasSubstr("42, we only allow 10"));
}
/// [Sample direct construction]

/// [Sample construction HTTP]
TEST(CustomHandlerException, DirectHttpSample) {
  auto exc = server::http::CustomHandlerException(
      server::handlers::HandlerErrorCode::kClientError,
      server::http::HttpStatus::kUnprocessableEntity,
      server::handlers::ExternalBody{"The provided password is too short"});

  EXPECT_EQ(exc.GetCode(), server::handlers::HandlerErrorCode::kClientError);
  EXPECT_EQ(server::http::GetHttpStatus(exc),
            server::http::HttpStatus::kUnprocessableEntity);
  EXPECT_EQ(exc.GetExternalErrorBody(), "The provided password is too short");
  EXPECT_THAT(exc.what(), testing::HasSubstr("Client error"));
}
/// [Sample construction HTTP]

USERVER_NAMESPACE_END
