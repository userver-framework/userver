#include <userver/server/handlers/auth/auth_digest_checker_stand_alone.hpp>
#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/handlers/auth/digest_context.hpp>
#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <exception>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::test {

class StandAloneChecker : public AuthCheckerDigestBaseStandAlone {
 public:
  StandAloneChecker(const AuthDigestSettings& digest_settings, Realm&& realm)
      : AuthCheckerDigestBaseStandAlone(digest_settings, std::move(realm)){};

  std::optional<HA1> GetHA1(const std::string&) const override {
    return HA1{"939e7578ed9e3c518a452acee763bce9"};
  }
  void PushUnnamedNonce(const Nonce&) const override;
  bool HasUnnamedNonce(const Nonce&) const override { return false; };
};

class DigestCalculation : public ::testing::Test {
    StandAloneChecker checker;
    DigestContextFromClient default_context;
    DigestContextFromClient client_context;
 public:
  DigestCalculation(const AuthDigestSettings& digest_settings, Realm&& realm) : 
    checker(digest_settings, realm),
    
  {

  }
  void SetUp(const AuthDigestSettings& digest_settings, Realm&& realm) {
    
  }
  void SetDefault() {
    client_context = default_context;
  }
}

TEST(DigestHashChecker, CalculateDigest) {
  StandAloneChecker checker;
  auto method = server::http::HttpMethod::kGet;
  DigestContextFromClient client_context;
  client_context.username = "Mufasa";
  client_context.realm = "testrealm@host.com";
  client_context.cnonce = "0a4f113b";
  client_context.nc = "00000001";
  client_context.uri = "/dir/index.html";
  client_context.qop = "auth";
  EXPECT_EQ(checker.CalculateDigest(method, client_context),
            "6629fae49393a05397450978507c4ef1");
  client_context.username = "Mubasa";
  EXPECT_NE(checker.CalculateDigest(method, client_context),
            "6629fae49393a05397450978507c4ef1");
  client_context.username = "Mufasa";
  client_context.username = 
}

}  // namespace server::handlers::auth::test

USERVER_NAMESPACE_END