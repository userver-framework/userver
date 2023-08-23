#include <userver/utest/utest.hpp>

#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/server/handlers/auth/auth_digest_checker_standalone.hpp>
#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/handlers/auth/digest_context.hpp>
#include <userver/server/handlers/auth/digest_checker_base.hpp>

#include <exception>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN


namespace server::handlers::auth::test {

using HA1 = utils::NonLoggable<class HA1Tag, std::string>;
using NonceCache = cache::ExpirableLruCache<std::string, TimePoint>;
using ValidateResult = DigestCheckerBase::ValidateResult;

class StandAloneChecker : public AuthCheckerDigestBaseStandalone {

 public:
  StandAloneChecker(const AuthDigestSettings& digest_settings, std::string&& realm)
      : AuthCheckerDigestBaseStandalone(digest_settings, std::move(realm)), nonce_cache(1, 1){
        nonce_cache.SetMaxLifetime(digest_settings.nonce_ttl);
      };

  std::optional<HA1> GetHA1(std::string_view) const override {
    return HA1{"939e7578ed9e3c518a452acee763bce9"};
  }

  void PushUnnamedNonce(std::string nonce, std::chrono::milliseconds nonce_ttl) const override {
    auto creation_time = userver::utils::datetime::Now();
    nonce_cache.Put(nonce, creation_time);
  };

  std::optional<TimePoint> GetUnnamedNonceCreationTime(const std::string& nonce) const override {
    auto nonce_creation_time = nonce_cache.GetOptionalNoUpdate(nonce);
    return nonce_creation_time;
  };

 private:
  mutable NonceCache nonce_cache;
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
      correct_client_context(
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
          "auth-param"}) {
            client_context_ = correct_client_context;
          }
  
 protected:
  std::string valid_nonce = "dcd98b7102dd2f0e8b11d0f600bfb0c093";
  std::string validHA1 = "939e7578ed9e3c518a452acee763bce9";
  AuthDigestSettings digest_settings_;
  StandAloneChecker checker_;
  DigestContextFromClient client_context_;
  DigestContextFromClient correct_client_context;
};

UTEST_F(StandAloneCheckerTest, SessionLogic) {
  utils::datetime::MockNowSet(std::chrono::system_clock::now());
  checker_.PushUnnamedNonce(valid_nonce, {});
  UserData test_data(HA1(validHA1), valid_nonce, utils::datetime::Now());
  utils::datetime::MockSleep(std::chrono::milliseconds(2));
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kOk);
  utils::datetime::MockSleep(std::chrono::milliseconds(20000));
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kWrongUserData);
}

UTEST_F(StandAloneCheckerTest, NonceCount) {
  checker_.PushUnnamedNonce(valid_nonce, {});
  UserData test_data(HA1(validHA1), valid_nonce, utils::datetime::Now());
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kOk);
  test_data.nonce_count++;
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kDuplicateRequest);
  correct_client_context.nc = "00000002";
  client_context_ = correct_client_context;
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kOk);
}

UTEST_F(StandAloneCheckerTest, InvalidNonce) {
  auto invalid_nonce = "abc88743bacdf9238";
  UserData test_data(HA1(validHA1), invalid_nonce, utils::datetime::Now());
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kWrongUserData);
  test_data.nonce = valid_nonce;
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data), ValidateResult::kOk);
}
}  // namespace server::handlers::auth::test

USERVER_NAMESPACE_END