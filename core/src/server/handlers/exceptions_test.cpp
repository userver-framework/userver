#include <userver/server/handlers/exceptions.hpp>

#include <gtest/gtest.h>

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

TEST(CustomErrorBuilder, Sample) {
  auto exc = server::handlers::ClientError(
      CustomErrorBuilder{"Bad Request", "Something went wrong"});

  EXPECT_EQ(exc.GetCode(), server::handlers::HandlerErrorCode::kClientError);
  EXPECT_EQ(exc.what(), std::string{"Client error"}) << "Log message";

  auto json = formats::json::FromString(
      R"({"code":"Bad Request","message":"Something went wrong"})");
  EXPECT_EQ(exc.GetExternalErrorBody(), ToString(json));
}
/// [Sample custom error builder]

USERVER_NAMESPACE_END
