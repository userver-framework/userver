#include <userver/utest/utest.hpp>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/flags.hpp>

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

class Messenger final : public sample::ugrpc::MessengerBase {
public:
    void Send(SendCall& call, sample::ugrpc::SendRequest&& /*request*/) override {
        sample::ugrpc::SendResponse response;
        response.set_delivered(true);
        response.mutable_reply()->set_text(grpc::string{kResponseText});
        response.set_token(grpc::string{kToken});

        call.Finish(response);
    }
};

enum class MiddlewareFlag {
    kNone = 0,
    kClientLog = 1 << 0,
    kServerLog = 1 << 1,
};

using MiddlewareFlags = utils::Flags<MiddlewareFlag>;

class SecretFieldsServiceFixture : public ugrpc::tests::ServiceFixtureBase,
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
            client_log_settings.log_level = logging::Level::kInfo;
            client_log_settings.msg_log_level = logging::Level::kInfo;
            SetClientMiddlewareFactories(
                {std::make_shared<ugrpc::client::middlewares::log::MiddlewareFactory>(client_log_settings)}
            );
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

    const auto response = client.Send(request).Finish();
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
    EXPECT_FALSE(logs_contain(kPassword)) << all_logs;
    EXPECT_FALSE(logs_contain(kSecretCode)) << all_logs;
    EXPECT_TRUE(logs_contain(kDest)) << all_logs;
    EXPECT_TRUE(logs_contain(kRequestText)) << all_logs;

    EXPECT_TRUE(logs_contain(kResponseText)) << all_logs;
    EXPECT_FALSE(logs_contain(kToken)) << all_logs;
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    SecretFieldsTest,
    testing::Values(MiddlewareFlags{MiddlewareFlag::kClientLog}, MiddlewareFlags{MiddlewareFlag::kServerLog})
);

USERVER_NAMESPACE_END
