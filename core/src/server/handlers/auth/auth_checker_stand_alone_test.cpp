#include <userver/server/handlers/auth/auth_digest_checker_stand_alone.hpp>
#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/handlers/auth/digest_context.hpp>
#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <exception>
#include <string_view>

class StandAloneChecker : public AuthCheckerDigestBaseStandAlone{
    public:
    using HA1 = utils::NonLoggable<class HA1Tag, std::string>;
    std::optional<HA1> GetHA1(const std::string& username) const override {
        return "939e7578ed9e3c518a452acee763bce9";
    }
}

// std::optional<std::string> AuthCheckerDigestBase::CalculateDigest(
//     const server::http::HttpMethod& request_method,
//     const DigestContextFromClient& client_context) const

TEST(DigestHashChecker, CalculateDigest) {
    StandAloneChecker checker;
    userver::server::http::HttpMethod method = userver::server::http::HttpMethod::kGet;
    server::handlers::auth::DigestContextFromClient client_context;
    client_context.username = "Mufasa";
    client_context.realm = "testrealm@host.com";
    client_context.cnonce = "0a4f113b";
    client_context.nc = "00000001";
    client_context.uri = "/dir/index.html";
    client_context.qop = "auth";
    EXPECT_EQ(checker.CalculateDigest(method, client_context),"6629fae49393a05397450978507c4ef1");
}