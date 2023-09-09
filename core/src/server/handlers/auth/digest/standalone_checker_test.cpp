#include <exception>
#include <string_view>
#include <vector>

#include <userver/utest/utest.hpp>

#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_base.hpp>
#include <userver/server/handlers/auth/digest/context.hpp>
#include <userver/server/handlers/auth/digest/directives_parser.hpp>
#include <userver/server/handlers/auth/digest/standalone_checker.hpp>
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
  StandAloneChecker(const AuthCheckerSettings& digest_settings,
                    std::string&& realm, const SecdistConfig& secdist_config)
      : AuthStandaloneCheckerBase(digest_settings, std::move(realm),
                                  secdist_config, kWays, kWaySize) {}

  std::optional<HA1> GetHA1(std::string_view) const override {
    return kValidHA1;
  }
};

class StandAloneCheckerTest : public ::testing::Test {
 public:
  struct TempFileProvider {
    TempFileProvider() {
      fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistJson);
    }
    const std::string& GetFilePath() const { return temp_file.GetPath(); }

    static constexpr std::string_view kSecdistJson = R"~(
    {
        "server-secret-key": "some-private-key"
    }
    )~";
    fs::blocking::TempFile temp_file{fs::blocking::TempFile::Create()};
  };

  StandAloneCheckerTest()
      : default_loader({temp_file_provier.GetFilePath(),
                        storages::secdist::SecdistFormat::kJson, true,
                        std::nullopt}),
        secdist_config({&default_loader, std::chrono::milliseconds::zero()}),
        digest_settings_({
            "MD5",                             // algorithm
            std::vector<std::string>{"/"},     // domains
            std::vector<std::string>{"auth"},  // qops
            false,                             // is_proxy
            false,                             // is_session
            kNonceTTL                          // nonce_ttl
        }),
        checker_(digest_settings_, "testrealm@host.com", secdist_config),
        correct_client_context_({
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
        }) {
    client_context_ = correct_client_context_;
  }

  TempFileProvider temp_file_provier;
  storages::secdist::DefaultLoader default_loader;
  storages::secdist::SecdistConfig secdist_config;

  AuthCheckerSettings digest_settings_;
  StandAloneChecker checker_;
  ContextFromClient client_context_;
  ContextFromClient correct_client_context_;
};

UTEST_F(StandAloneCheckerTest, NonceTTL) {
  utils::datetime::MockNowSet(utils::datetime::Now());
  checker_.PushUnnamedNonce(kValidNonce);

  UserData test_data{kValidHA1, kValidNonce, utils::datetime::Now(), 0};
  utils::datetime::MockSleep(kNonceTTL - std::chrono::milliseconds(100));
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kOk);

  utils::datetime::MockSleep(kNonceTTL + std::chrono::milliseconds(100));
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kWrongUserData);
}

UTEST_F(StandAloneCheckerTest, NonceCount) {
  checker_.PushUnnamedNonce(kValidNonce);

  UserData test_data{kValidHA1, kValidNonce, utils::datetime::Now(), 0};
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kOk);

  test_data.nonce_count++;
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kDuplicateRequest);

  correct_client_context_.nc = "00000002";
  client_context_ = correct_client_context_;
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kOk);
}

UTEST_F(StandAloneCheckerTest, InvalidNonce) {
  const auto* invalid_nonce_ = "abc88743bacdf9238";
  UserData test_data{kValidHA1, invalid_nonce_, utils::datetime::Now(), 0};
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kWrongUserData);

  test_data.nonce = kValidNonce;
  EXPECT_EQ(checker_.ValidateUserData(client_context_, test_data),
            ValidateResult::kOk);
}

UTEST_F(StandAloneCheckerTest, NonceCountConvertingThrow) {
  client_context_.nc = "not-a-hex-number";
  UserData test_data{kValidHA1, kValidNonce, utils::datetime::Now(), 0};
  EXPECT_THROW(checker_.ValidateUserData(client_context_, test_data),
               std::runtime_error);
}

}  // namespace server::handlers::auth::digest::test

USERVER_NAMESPACE_END
