#include <set>
#include <userver/server/handlers/auth/auth_digest_checker_standalone.hpp>
#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/handlers/auth/digest_context.hpp>

#include <gtest/gtest.h>

#include <exception>
#include <string_view>
#include <vector>
#include <userver/server/handlers/auth/auth_digest_checker_base.hpp>
#include "userver/utils/datetime.hpp"
#include "userver/utils/mock_now.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::test {

class StandAloneChecker : public AuthCheckerDigestBaseStandalone {
 public:
  StandAloneChecker(const AuthDigestSettings& digest_settings, Realm&& realm)
      : AuthCheckerDigestBaseStandalone(digest_settings, std::move(realm)){};

  std::optional<HA1> GetHA1(const std::string&) const override {
    return HA1{"939e7578ed9e3c518a452acee763bce9"};
  }

  void PushUnnamedNonce(const Nonce&, std::chrono::milliseconds) const override {};

  std::optional<TimePoint> GetUnnamedNonceCreationTime(const Nonce& nonce) const override {
    if (unnamed_nonce_storage_.count(nonce)) return utils::datetime::Now();
    return std::nullopt;
  };

 private:
  std::set<std::string> unnamed_nonce_storage_{"existing-nonce"};
};

class StandAloneCheckerTest : public ::testing::Test {
 public:
  StandAloneCheckerTest()
    : digest_settings_(
        AuthDigestSettings{
          "MD5",
          std::vector<std::string>{ "/" },
          std::vector<std::string>{ "auth" }, 
          false,
          false,
          std::chrono::milliseconds{1000}}),
      checker_(digest_settings_, "registred@userver.com"),
      client_context_(
        DigestContextFromClient{
          "Mufasa",
          "testrealm@host.com",
          "3f93a38e2fdb46e36dc74e0e4b221ca4",
          "/dir/index.html",
          "response",
          "MD5",
          "bea007ff2c14c8fbec8eeaafab264f16",
          "00000001",
          "auth",
          "auth-param"}) {}

 protected:

  AuthDigestSettings digest_settings_;
  StandAloneChecker checker_;
  DigestContextFromClient client_context_;
};

TEST_F(StandAloneCheckerTest, DirectiveValidation) {
  client_context_.nonce = "no-existing-nonce";
  EXPECT_EQ(checker_.ValidateClientData(client_context_), ValidateClientDataResult::kWrongUserData);
 
  // utils::datetime::MockNowSet(utils::datetime::Now());
  // utils::datetime::MockSleep(9001);  // utils::datetime::MockNowSet(utils::datetime::Now());
  // utils::datetime::MockSleep(9001);
  // EXPECT_EQ(timer.NextLoop(), 9001s);
 
  // utils::datetime::MockNowSet(Stringtime("2000-01-02T00:00:00+0000"));
  // EXPECT_EQ(timer.NextLoop(), 24h - 9001s);
  // EXPECT_EQ(timer.NextLoop(), 9001s);
 
  // utils::datetime::MockNowSet(Stringtime("2000-01-02T00:00:00+0000"));
  // EXPECT_EQ(timer.NextLoop(), 24h - 9001s);
  // client_context_.nonce = "existing-nonce";
  // EXPECT_EQ(checker_.ValidateClientData(client_context_), ValidateClientDataResult::kWrongUserData);
}

}  // namespace server::handlers::auth::test

USERVER_NAMESPACE_END