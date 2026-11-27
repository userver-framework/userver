#include <clients/expected_status_codes/client_impl.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <utility>

#include <userver/chaotic/openapi/client/command_control.hpp>
#include <userver/chaotic/openapi/client/exceptions.hpp>
#include <userver/clients/http/expected_status_codes.hpp>
#include <userver/http/status_code.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr chaotic::openapi::client::CommandControl
    kAggregateInitializedCommandControl{std::chrono::milliseconds{400}, 3};
static_assert(!kAggregateInitializedCommandControl.expected_status_codes.has_value());

}  // namespace

class ExpectedCodegenStatusCodes : public utest::LogCaptureFixture<> {
protected:
    template <typename Call>
    void Check(Call&& call, std::optional<clients::http::ExpectedStatusCodes> per_call, bool is_error) {
        chaotic::openapi::client::CommandControl cc;
        cc.expected_status_codes = std::move(per_call);
        EXPECT_THROW(call(cc), chaotic::openapi::client::ExceptionWithStatusCode);
        const auto spans = GetLogCapture().Filter([](const utest::LogRecord& record) {
            return record.GetTagOptional("stopwatch_name").has_value() && record.GetTagOptional("meta_code") == "404";
        });
        ASSERT_EQ(spans.size(), 1);
        EXPECT_EQ(spans.front().GetTagOptional("error"), is_error ? std::optional<std::string>{"1"} : std::nullopt);
        GetLogCapture().Clear();
    }
};

UTEST_F(ExpectedCodegenStatusCodes, ClientOperationAndPerCallReplaceLists) {
    const utest::HttpServerMock server{[](const auto&) { return utest::HttpServerMock::HttpResponse{404, {}, {}}; }};
    auto http_client = utest::CreateHttpClient();
    ::clients::expected_status_codes::ClientImpl client{{server.GetBaseUrl()}, *http_client};

    const auto inherited = [&client](const auto& cc) { return client.Inherited(cc); };
    const auto overridden = [&client](const auto& cc) { return client.Overridden(cc); };
    const auto cleared = [&client](const auto& cc) { return client.Cleared(cc); };

    Check(overridden, std::nullopt, true);
    Check(cleared, std::nullopt, true);

    //! [expected-status-codes-per-call]
    using ExpectedStatusCodes = USERVER_NAMESPACE::clients::http::ExpectedStatusCodes;
    using StatusCode = USERVER_NAMESPACE::http::StatusCode;

    USERVER_NAMESPACE::chaotic::openapi::client::CommandControl cc;
    cc.expected_status_codes = ExpectedStatusCodes{StatusCode::kNotFound};
    //! [expected-status-codes-per-call]
    Check(overridden, cc.expected_status_codes, false);
    Check(cleared, cc.expected_status_codes, false);

    //! [expected-status-codes-clear]
    cc.expected_status_codes = ExpectedStatusCodes{};
    //! [expected-status-codes-clear]
    Check(inherited, cc.expected_status_codes, true);

    //! [expected-status-codes-inherit]
    cc.expected_status_codes = std::nullopt;
    //! [expected-status-codes-inherit]
    Check(inherited, cc.expected_status_codes, false);
    Check(inherited, clients::http::ExpectedStatusCodes{USERVER_NAMESPACE::http::StatusCode::kConflict}, true);
}

USERVER_NAMESPACE_END
