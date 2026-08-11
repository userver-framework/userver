#include <google/protobuf/stubs/common.h>
#if defined(ARCADIA_ROOT) || GOOGLE_PROTOBUF_VERSION >= 4022000

#include <gmock/gmock.h>

#include <userver/ugrpc/protobuf_logging.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/flags.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>

#include <tests/secret_fields.pb.h>
#include <tests/secret_fields_client.usrv.pb.hpp>
#include <tests/secret_fields_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kLogin = "login-value";
constexpr std::string_view kPassword = "password-value";
constexpr std::string_view kSecretCode = "secret-code-value";

constexpr std::string_view kDest = "dest-value";
constexpr std::string_view kRequestText = "request-text-value";

constexpr std::string_view kResponseText = "response-text-value";
constexpr std::string_view kToken = "token-value";

constexpr std::string_view kTruncateMarker = "...(truncated)";

class Messenger final : public sample::ugrpc::MessengerBase {
public:
    SendResult Send(CallContext& /*context*/, sample::ugrpc::SendRequest&& /*request*/) override {
        sample::ugrpc::SendResponse response;
        response.set_delivered(true);
        response.mutable_reply()->set_text(grpc::string{kResponseText});
        response.set_token(grpc::string{kToken});

        return response;
    }
};

enum class MiddlewareFlag {
    kNone = 0,
    kClientLog = 1 << 0,
    kServerLog = 1 << 1,
};

using MiddlewareFlags = utils::Flags<MiddlewareFlag>;

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class SecretFieldsServiceFixture
    : public ugrpc::tests::ServiceFixtureBase,
      public testing::WithParamInterface<MiddlewareFlags> {
protected:
    SecretFieldsServiceFixture() {
        if (GetParam() & MiddlewareFlag::kServerLog) {
            ugrpc::server::middlewares::log::Settings server_log_settings;
            server_log_settings.msg_log_level = logging::Level::kInfo;
            SetServerMiddlewares({std::make_shared<ugrpc::server::middlewares::log::Middleware>(server_log_settings)});
        }

        if (GetParam() & MiddlewareFlag::kClientLog) {
            ugrpc::client::middlewares::log::Settings client_log_settings;
            client_log_settings.msg_log_level = logging::Level::kInfo;
            SetClientMiddlewares({std::make_shared<ugrpc::client::middlewares::log::Middleware>(client_log_settings)});
        }

        RegisterService(service_);
        StartServer();
    }

private:
    Messenger service_;
};

using SecretFieldsTest = utest::LogCaptureFixture<SecretFieldsServiceFixture>;

bool ContainInLog(const utest::LogCaptureLogger& log_capture, std::string_view needle) {
    const auto filtered = log_capture.Filter([needle](const auto& log_record) {
        const auto& log_raw = log_record.GetLogRaw();
        return std::string_view::npos != log_raw.find(needle);
    });
    return !filtered.empty();
}

}  // namespace

UTEST_P(SecretFieldsTest, MiddlewaresHideSecrets) {
    const auto client = MakeClient<sample::ugrpc::MessengerClient>();

    sample::ugrpc::SendRequest request;
    request.mutable_creds()->set_login(grpc::string{kLogin});
    request.mutable_creds()->set_password(grpc::string{kPassword});
    request.mutable_creds()->set_secret_code(grpc::string{kSecretCode});
    request.set_dest(grpc::string{kDest});
    request.mutable_msg()->set_text(grpc::string{kRequestText});

    const auto response = client.Send(request);
    EXPECT_EQ(true, response.delivered());
    EXPECT_EQ(kResponseText, response.reply().text());
    EXPECT_EQ(kToken, response.token());

    // Ensure that server logs get written.
    GetServer().StopServing();

    const auto logs_contain = [&log_capture = GetLogCapture()](std::string_view needle) {
        return ContainInLog(log_capture, needle);
    };

    const auto all_logs = GetLogCapture().GetAll();

    EXPECT_TRUE(logs_contain(kLogin)) << all_logs;
    EXPECT_TRUE(!logs_contain(kPassword)) << all_logs;
    EXPECT_TRUE(!logs_contain(kSecretCode)) << all_logs;
    EXPECT_TRUE(logs_contain(kDest)) << all_logs;
    EXPECT_TRUE(logs_contain(kRequestText)) << all_logs;

    EXPECT_TRUE(logs_contain(kResponseText)) << all_logs;
    EXPECT_TRUE(!logs_contain(kToken)) << all_logs;
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    SecretFieldsTest,
    testing::Values(
        MiddlewareFlags{MiddlewareFlag::kClientLog},
        MiddlewareFlags{MiddlewareFlag::kServerLog},
        MiddlewareFlags{MiddlewareFlag::kClientLog},
        MiddlewareFlags{MiddlewareFlag::kServerLog}
    )
);

UTEST(ToLimitedLoggingStringWithSecrets, Basic) {
    constexpr std::size_t kLimit = 2000;
    sample::ugrpc::LoggedMessageWithSecrets message;

    message.set_id("swag");
    message.set_password("qwerty12345678");
    message.set_name("test-name");
    message.set_count(7);
    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    const std::string expected = R"({"id":"swag","name":"test-name","password":"[REDACTED]","count":"[REDACTED]"})";
    EXPECT_EQ(out, expected);

    const auto truncated = ugrpc::ToLimitedLoggingString(message, 44);
    EXPECT_THAT(truncated, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(truncated, testing::HasSubstr(R"("id":"swag")"));
}

UTEST(ToLimitedLoggingStringWithSecrets, InnerSecrets) {
    constexpr std::size_t kLimit = 2000;
    sample::ugrpc::LoggedMessageWithSecrets message;

    message.set_id("swag");

    auto* item = message.add_items();
    item->set_index(7);
    item->set_value("secret");
    const std::string expected = R"({"id":"swag","items":[{"index":7,"value":"[REDACTED]"}]})";

    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_EQ(out, expected);
}

UTEST(ToLimitedLoggingStringWithSecrets, SecretsAndFit) {
    constexpr std::size_t kLimit = 200;
    sample::ugrpc::LoggedMessageWithSecrets message;

    message.set_id("swag");

    auto* item = message.add_items();
    item->set_index(7);
    item->set_value("long long long long long long long secret");
    const std::string_view expected = R"({"id":"swag","items":[{"index":7,"value":"[REDACTED]"}]})";

    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_EQ(out, expected);
}

UTEST(ToLimitedLoggingStringWithSecrets, SecretsAndTruncated) {
    constexpr std::size_t kLimit = 50;
    sample::ugrpc::LoggedMessageWithSecrets message;

    message.set_id("swag");

    auto* item = message.add_items();
    item->set_index(7);
    item->set_value("long long long long long long long secret");

    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(out, testing::HasSubstr(R"("id":"swag")"));
    EXPECT_THAT(out, testing::HasSubstr(R"("items")"));
}

UTEST(ToLimitedLoggingStringWithSecrets, TruncatedSecrets) {
    constexpr std::size_t kLimit = 62;
    sample::ugrpc::LoggedMessageWithSecrets message;

    message.set_id("swag");
    message.set_password("qwerty12345678");
    message.set_name("test-name");
    message.set_count(7);

    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_THAT(out, testing::EndsWith(kTruncateMarker));
    EXPECT_THAT(out, testing::HasSubstr(R"("id":"swag")"));
    EXPECT_THAT(out, testing::HasSubstr("[REDACTED]"));
}

UTEST(ToLimitedLoggingStringWithSecrets, TheSameName) {
    constexpr std::size_t kLimit = 200;
    sample::ugrpc::LoggedMessageWithSecrets message;

    message.set_name("name1");

    auto* item = message.add_items();
    item->set_name("name2");

    const std::string expected = R"({"name":"name1","items":[{"name":"name2"}]})";

    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_EQ(out, expected);
}

UTEST(ToLimitedLoggingStringWithSecrets, StructSecret) {
    constexpr std::size_t kLimit = 200;
    sample::ugrpc::StructSecret message;
    message.set_name1("name1");
    message.set_name2("name2");

    auto& current_state = *message.mutable_current_state();
    auto& fields = *current_state.mutable_fields();
    fields["key"].set_string_value("value");

    const std::string expected = R"({"name1":"name1","current_state":"[REDACTED]","name2":"name2"})";

    const auto out = ugrpc::ToLimitedLoggingString(message, kLimit);
    EXPECT_EQ(out, expected);
}

USERVER_NAMESPACE_END

#endif
