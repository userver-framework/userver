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

  void PushUnnamedNonce(const Nonce& nonce) const override {
    unnamed_nonce_storage[nonce] = userver::utils::datetime::Now() + nonce_ttl_;
  };

  std::optional<TimePoint> GetUnnamedNonceCreationTime(const Nonce& nonce) const override {
    if (unnamed_nonce_storage_.count(nonce)) return unnamed_nonce_storage_[nonce];
    return std::nullopt;
  };

 private:
  mutable std::unordered_map<Nonce, TimePoint> 
    unnamed_nonce_storage_;
  //std::set<std::string> unnamed_nonce_storage_{"existing-nonce"};
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
      checker_(digest_settings_, "testrealm@host.com"),
      client_context_(
        DigestContextFromClient{
          "Mufasa",
          "testrealm@host.com",
          "dcd98b7102dd2f0e8b11d0f600bfb0c093",
          "/dir/index.html",
          "6629fae49393a05397450978507c4ef1",
          "MD5",
          "0a4f113b",
          "5ccc069c403ebaf9f0171e9517f40e41",
          "auth",
          "00000001",
          "auth-param"}) {}

 protected:

  AuthDigestSettings digest_settings_;
  StandAloneChecker checker_;
  DigestContextFromClient client_context_;
};

TEST_F(StandAloneCheckerTest, DirectiveValidation) {
  // пришел пустой запрос, ответили 401, кинули в пул новый nonce 
  checker_.PushUnnamedNonce("939e7578ed9e3c518a452acee763bce9")
  // ждем ответа 
  client_context_.nonce = "939e7578ed9e3c518a452acee763bce9";
  EXPECT_EQ(checker_.ValidateUserData(client_context_), ValidateResult::kOk);
  client_context_.nonce = "just wrong";
  EXPECT_EQ(checker_.ValidateUserData(client_context_), ValidateResult::kWrongUserData);
  
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
  // EXPECT_EQ(checker_.ValidateUserData(client_context_), ValidateUserDataResult::kWrongUserData);
}

}  // namespace server::handlers::auth::test

USERVER_NAMESPACE_END