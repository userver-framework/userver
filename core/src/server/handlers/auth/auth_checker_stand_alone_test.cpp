#include <optional>
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

  std::optional<HA1> GetHA1(const std::string& username) const override {
    if (registred_users_storage_.count(username)) return HA1{registred_users_storage_.at(username)};
    return std::nullopt;
  }

  void PushUnnamedNonce(const Nonce&, std::chrono::milliseconds) const override {};

  std::optional<TimePoint> GetUnnamedNonceCreationTime(const Nonce& nonce) const override {
    if (unnamed_nonce_storage_.count(nonce)) return utils::datetime::Now();
    return std::nullopt;
  };

 private:
  std::set<std::string> unnamed_nonce_storage_{"existing-nonce"};
  std::map<std::string, std::string> registred_users_storage_{
    {"registred_user", "939e7578ed9e3c518a452acee763bce9"},
    };
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
          "registred_user",
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
  // utils::datetime::MockSleep(9001);
  // EXPECT_EQ(timer.NextLoop(), 9001s);
 
  // utils::datetime::MockNowSet(Stringtime("2000-01-02T00:00:00+0000"));
  // EXPECT_EQ(timer.NextLoop(), 24h - 9001s);
  // client_context_.nonce = "existing-nonce";
  // EXPECT_EQ(checker_.ValidateClientData(client_context_), ValidateClientDataResult::kOk);
}

// TEST(DigestHashChecker, CalculateDigest) {
//   AuthDigestSettings digest_settings;
//   digest_settings.algorithm = "MD5";
//   digest_settings.domains = std::vector<std::string>{ "/" };
//   digest_settings.is_proxy = false;
//   digest_settings.is_session = false;
//   digest_settings.nonce_ttl = std::chrono::milliseconds{1000};
//   digest_settings.qops = std::vector<std::string>{ "auth" };

//   StandAloneChecker checker(digest_settings, "registred@userver.com");

//   DigestContextFromClient client_context;
//   client_context.username = "Mufasa";
//   client_context.realm = "testrealm@host.com";
//   client_context.cnonce = "0a4f113b";
//   client_context.nc = "00000001";
//   client_context.uri = "/dir/index.html";
//   client_context.qop = "auth";
//   EXPECT_EQ(checker.CalculateDigest(http::HttpMethod::kGet, client_context),
//             "6629fae49393a05397450978507c4ef1");
// }

}  // namespace server::handlers::auth::test

USERVER_NAMESPACE_END