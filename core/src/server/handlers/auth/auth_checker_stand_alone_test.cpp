#include <userver/server/handlers/auth/auth_digest_checker_standalone.hpp>
#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/handlers/auth/digest_context.hpp>

#include <gtest/gtest.h>

#include <exception>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::test {

class StandAloneChecker : public AuthCheckerDigestBaseStandalone {
 public:
  StandAloneChecker(const AuthDigestSettings& digest_settings, Realm&& realm)
      : AuthCheckerDigestBaseStandalone(digest_settings, std::move(realm)){};

  std::optional<HA1> GetHA1(const std::string&) const override {
    return HA1{"939e7578ed9e3c518a452acee763bce9"};
  }
  void PushUnnamedNonce(const Nonce&) const override {};
  bool HasUnnamedNonce(const Nonce&) const override { return false; };
};

// std::optional<std::string> AuthCheckerDigestBase::CalculateDigest(
//     const server::http::HttpMethod& request_method,
//     const DigestContextFromClient& client_context) const

TEST(DigestHashChecker, CalculateDigest) {
  AuthDigestSettings digest_settings;
  digest_settings.algorithm = "MD5";
  digest_settings.domains = std::vector<std::string>{ "/" };
  digest_settings.is_proxy = false;
  digest_settings.is_session = false;
  digest_settings.nonce_ttl = std::chrono::milliseconds{1000};
  digest_settings.qops = std::vector<std::string>{ "auth" };

  StandAloneChecker checker(digest_settings, "registred@userver.com");

  auto method = http::HttpMethod::kGet;
  DigestContextFromClient client_context;
  client_context.username = "Mufasa";
  client_context.realm = "testrealm@host.com";
  client_context.cnonce = "0a4f113b";
  client_context.nc = "00000001";
  client_context.uri = "/dir/index.html";
  client_context.qop = "auth";
  EXPECT_EQ(checker.CalculateDigest(method, client_context),
            "6629fae49393a05397450978507c4ef1");

  checker.CalculateDigest(method, client_context)
    Which is: ("0472085a9fdeabcc788c5c8a6f0d93d8")
  "6629fae49393a05397450978507c4ef1"
    Which is: 0x5607b8ca0400
}

}  // namespace server::handlers::auth::test

USERVER_NAMESPACE_END