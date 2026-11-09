#include <exception>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <userver/utest/utest.hpp>

#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_base.hpp>
#include <userver/server/handlers/auth/digest/context.hpp>
#include <userver/server/handlers/auth/digest/directives_parser.hpp>
#include <userver/server/handlers/auth/digest/standalone_checker.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest::test {

using HA1 = utils::NonLoggable<class HA1Tag, std::string>;
using NonceCache = cache::ExpirableLruCache<std::string, TimePoint>;
using ValidateResult = AuthCheckerBase::ValidateResult;

constexpr std::size_t kWays = 4;
constexpr std::size_t kWaySize = 25000;

// hash of `username:realm:password` for testing
// each user is considered registered
const auto kValidHA1 = HA1{"939e7578ed9e3c518a452acee763bce9"};
const std::string kValidNonce = "dcd98b7102dd2f0e8b11d0f600bfb0c093";
constexpr auto kNonceTTL = std::chrono::milliseconds{1000};

class StandAloneChecker final : public AuthStandaloneCheckerBase {
public:
    StandAloneChecker(
        const AuthCheckerSettings& digest_settings,
        std::string&& realm,
        const SecdistConfig& secdist_config
    )
        : AuthStandaloneCheckerBase(
              digest_settings,
              std::move(realm),
              secdist_config,
              NonceCacheSettings{kWays, kWaySize}
          )
    {}

    std::optional<HA1> GetHA1(std::string_view) const override { return kValidHA1; }
};

class StandAloneCheckerTest : public ::testing::Test {
public:
    struct TempFileProvider {
        TempFileProvider() { fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistJson); }
        const std::string& GetFilePath() const { return temp_file.GetPath(); }

        static constexpr std::string_view kSecdistJson = R"~(
    {
        "http_server_digest_auth_secret": "some-private-key"
    }
    )~";
        fs::blocking::TempFile temp_file{fs::blocking::TempFile::Create()};
    };

    StandAloneCheckerTest()
        : default_loader(
              {temp_file_provider.GetFilePath(),
               storages::secdist::SecdistFormat::kJson,
               true,
               std::nullopt,
               nullptr,
               {}}
          ),
          secdist_config({&default_loader, std::chrono::milliseconds::zero()}),
          digest_settings({
              "MD5",                             // algorithm
              std::vector<std::string>{"/"},     // domains
              std::vector<std::string>{"auth"},  // qops
              false,                             // is_proxy
              false,                             // is_session
              kNonceTTL                          // nonce_ttl
          }),
          checker(digest_settings, "testrealm@host.com", secdist_config),
          correct_client_context({
              "Mufasa",                            // username
              "testrealm@host.com",                // realm
              kValidNonce,                         // nonce
              "/dir/index.html",                   // uri
              "6629fae49393a05397450978507c4ef1",  // response
              "MD5",                               // algorithm
              "0a4f113b",                          // cnonce
              "5ccc069c403ebaf9f0171e9517f40e41",  // opaque
              "auth",                              // qop
              "00000001",                          // nc
              "auth-param"                         // authparam
          })
    {
        client_context = correct_client_context;
    }

    TempFileProvider temp_file_provider;
    storages::secdist::DefaultLoader default_loader;
    storages::secdist::SecdistConfig secdist_config;

    AuthCheckerSettings digest_settings;
    StandAloneChecker checker;
    ContextFromClient client_context;
    ContextFromClient correct_client_context;
};

UTEST_F(StandAloneCheckerTest, NonceTTL) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    checker.PushUnnamedNonce(kValidNonce);

    const UserData test_data{kValidHA1, kValidNonce, utils::datetime::Now(), 0};
    utils::datetime::MockSleep(kNonceTTL - std::chrono::milliseconds(100));
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kOk);

    utils::datetime::MockSleep(kNonceTTL + std::chrono::milliseconds(100));
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kWrongUserData);
}

UTEST_F(StandAloneCheckerTest, NonceCount) {
    checker.PushUnnamedNonce(kValidNonce);

    UserData test_data{kValidHA1, kValidNonce, utils::datetime::Now(), 0};
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kOk);

    test_data.nonce_count++;
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kDuplicateRequest);

    correct_client_context.nc = "00000002";
    client_context = correct_client_context;
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kOk);
}

UTEST_F(StandAloneCheckerTest, InvalidNonce) {
    const auto* invalid_nonce = "abc88743bacdf9238";
    UserData test_data{kValidHA1, invalid_nonce, utils::datetime::Now(), 0};
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kWrongUserData);

    test_data.nonce = kValidNonce;
    EXPECT_EQ(checker.ValidateUserData(client_context, test_data), ValidateResult::kOk);
}

UTEST_F(StandAloneCheckerTest, NonceCountConvertingThrow) {
    client_context.nc = "not-a-hex-number";
    const UserData test_data{kValidHA1, kValidNonce, utils::datetime::Now(), 0};
    EXPECT_THROW(checker.ValidateUserData(client_context, test_data), std::runtime_error);
}

namespace {

std::shared_ptr<http::HttpRequest> MakeStateTestRequest(std::string nonce, std::string nc, std::string response) {
    return http::HttpRequestBuilder()
        .SetMethod(http::HttpMethod::kGet)
        .SetUrl("/dir/index.html")
        .AddHeader(
            "Authorization",
            fmt::format(
                R"(Digest username="Mufasa", realm="testrealm@host.com", nonce="{}", )"
                R"(uri="/dir/index.html", response="{}", qop=auth, nc={}, cnonce="0a4f113b")",
                nonce,
                response,
                nc
            )
        )
        .Build();
}

}  // namespace

UTEST_F(StandAloneCheckerTest, InvalidDigestDoesNotAdvanceNonceCount) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    const auto timestamp = utils::datetime::Now();
    checker.SetUserData("Mufasa", kValidNonce, 0, timestamp);
    auto request = MakeStateTestRequest(kValidNonce, "ffffffff", "invalid");
    request::RequestContext context;

    EXPECT_EQ(checker.CheckAuth(*request, context).status, AuthCheckResult::Status::kInvalidToken);

    const auto data = checker.FetchUserData("Mufasa");
    ASSERT_TRUE(data);
    EXPECT_EQ(data->nonce, kValidNonce);
    EXPECT_EQ(data->nonce_count, 0);
    EXPECT_EQ(data->timestamp, timestamp);
    auto valid_request = MakeStateTestRequest(kValidNonce, "00000001", "6629fae49393a05397450978507c4ef1");
    EXPECT_EQ(checker.CheckAuth(*valid_request, context).status, AuthCheckResult::Status::kOk);
}

UTEST_F(StandAloneCheckerTest, InvalidDigestDoesNotConsumeUnnamedNonce) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    checker.SetUserData("Mufasa", kValidNonce, 0, utils::datetime::Now());
    checker.PushUnnamedNonce("challenge");
    auto request = MakeStateTestRequest("challenge", "00000001", "invalid");
    request::RequestContext context;

    EXPECT_EQ(checker.CheckAuth(*request, context).status, AuthCheckResult::Status::kInvalidToken);
    EXPECT_TRUE(checker.GetUnnamedNonceCreationTime("challenge"));
    EXPECT_EQ(checker.FetchUserData("Mufasa")->nonce, kValidNonce);
}

UTEST_F(StandAloneCheckerTest, NewChallengeDoesNotReplaceUserSession) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    const auto timestamp = utils::datetime::Now();
    checker.SetUserData("Mufasa", kValidNonce, 7, timestamp);
    const Hasher hasher{"MD5", secdist_config};
    const auto ha2 = hasher.GetHash("GET:/dir/index.html");
    const auto digest = hasher.GetHash({kValidHA1.GetUnderlying(), ":unknown:00000001:0a4f113b:auth:", ha2});
    auto request = MakeStateTestRequest("unknown", "00000001", digest);
    request::RequestContext context;

    EXPECT_EQ(checker.CheckAuth(*request, context).status, AuthCheckResult::Status::kInvalidToken);

    const auto data = checker.FetchUserData("Mufasa");
    ASSERT_TRUE(data);
    EXPECT_EQ(data->nonce, kValidNonce);
    EXPECT_EQ(data->nonce_count, 7);
    EXPECT_EQ(data->timestamp, timestamp);
}

UTEST_F(StandAloneCheckerTest, InvalidDigestDoesNotReflectUnsafeNonce) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    const auto timestamp = utils::datetime::Now();

    for (const auto& nonce : {std::string{"bad\tnonce"}, std::string{R"(bad\"nonce)"}, std::string{R"(bad\\nonce)"}}) {
        SCOPED_TRACE(nonce);
        checker.SetUserData("Mufasa", kValidNonce, 7, timestamp);
        checker.PushUnnamedNonce("existing-challenge");
        auto request = MakeStateTestRequest(nonce, "ffffffff", "invalid");
        request::RequestContext context;

        EXPECT_EQ(checker.CheckAuth(*request, context).status, AuthCheckResult::Status::kInvalidToken);
        const auto& response = request->GetHttpResponse();
        EXPECT_EQ(response.GetStatus(), http::HttpStatus::kUnauthorized);
        const auto& challenge = response.GetHeader("WWW-Authenticate");
        EXPECT_EQ(challenge.find('\t'), std::string::npos);
        Parser parser;
        const auto client_nonce = parser.ParseAuthInfo(request->GetHeader("Authorization")).nonce;
        EXPECT_EQ(challenge.find(client_nonce), std::string::npos);
        EXPECT_NE(challenge.find(R"(stale="false")"), std::string::npos);

        const auto data = checker.FetchUserData("Mufasa");
        ASSERT_TRUE(data);
        EXPECT_EQ(data->nonce, kValidNonce);
        EXPECT_EQ(data->nonce_count, 7);
        EXPECT_EQ(data->timestamp, timestamp);
        EXPECT_TRUE(checker.GetUnnamedNonceCreationTime("existing-challenge"));
    }
}

UTEST_F(StandAloneCheckerTest, InvalidDigestReturnsUsableChallenge) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    checker.SetUserData("Mufasa", kValidNonce, 7, utils::datetime::Now());
    auto request = MakeStateTestRequest("bad\tnonce", "00000001", "invalid");
    request::RequestContext context;
    ASSERT_EQ(checker.CheckAuth(*request, context).status, AuthCheckResult::Status::kInvalidToken);

    const auto& challenge = request->GetHttpResponse().GetHeader("WWW-Authenticate");
    constexpr std::string_view kNoncePrefix = R"(nonce=")";
    const auto nonce_pos = challenge.find(kNoncePrefix);
    ASSERT_NE(nonce_pos, std::string::npos);
    const auto nonce_begin = nonce_pos + kNoncePrefix.size();
    const auto nonce_end = challenge.find('"', nonce_begin);
    ASSERT_NE(nonce_end, std::string::npos);
    const auto nonce = challenge.substr(nonce_begin, nonce_end - nonce_begin);
    ASSERT_FALSE(nonce.empty());
    const Hasher hasher{"MD5", secdist_config};
    const auto ha2 = hasher.GetHash("GET:/dir/index.html");
    const auto digest = hasher.GetHash({kValidHA1.GetUnderlying(), ":", nonce, ":00000001:0a4f113b:auth:", ha2});
    auto valid_request = MakeStateTestRequest(nonce, "00000001", digest);

    EXPECT_EQ(checker.CheckAuth(*valid_request, context).status, AuthCheckResult::Status::kOk);
}

UTEST_F(StandAloneCheckerTest, FreshChallengeReplacesExpiredSession) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    checker.SetUserData("Mufasa", "old-session", 7, utils::datetime::Now() - 2 * kNonceTTL);
    checker.PushUnnamedNonce(kValidNonce);
    auto request = MakeStateTestRequest(kValidNonce, "00000001", "6629fae49393a05397450978507c4ef1");
    request::RequestContext context;

    EXPECT_EQ(checker.CheckAuth(*request, context).status, AuthCheckResult::Status::kOk);
}

UTEST_F(StandAloneCheckerTest, FreshChallengeCommitsItsOwnReplayState) {
    utils::datetime::MockNowSet(utils::datetime::Now());
    const auto timestamp = utils::datetime::Now();
    const UserData previous{kValidHA1, "old-session", timestamp - 2 * kNonceTTL, 7};
    checker.PushUnnamedNonce(kValidNonce);

    EXPECT_EQ(checker.ValidateUserData(client_context, previous), ValidateResult::kOk);

    const auto data = checker.FetchUserData("Mufasa");
    ASSERT_TRUE(data);
    EXPECT_EQ(data->nonce, kValidNonce);
    EXPECT_EQ(data->nonce_count, 1);
    EXPECT_EQ(data->timestamp, timestamp);
}

}  // namespace server::handlers::auth::digest::test

USERVER_NAMESPACE_END
