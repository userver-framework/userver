#include <userver/utest/utest.hpp>

#include <array>
#include <ranges>
#include <system_error>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/http/client.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utils/async.hpp>

#include <engine/io/tests/net_listener.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

using ResponseQueue = concurrent::SpmcQueue<int>;

class WaitingEchoHandler final {
public:
    explicit WaitingEchoHandler(ResponseQueue& queue)
        : data_(std::make_shared<Data>(Data{queue.GetMultiConsumer()}))
    {}

    utest::HttpServerMock::HttpResponse operator()(const utest::HttpServerMock::HttpRequest& request) {
        int response_status{};
        const bool success = data_->responses.Pop(response_status);
        if (!success) {
            return {500, {}, ""};
        }

        utest::HttpServerMock::HttpResponse response{};
        response.response_status = response_status;
        response.body = request.body;
        response.headers = request.headers;
        return response;
    }

private:
    struct Data final {
        ResponseQueue::MultiConsumer responses;
    };

    std::shared_ptr<Data> data_;
};

class HttpClientDeadline : public ::testing::Test {
protected:
    void PushResponseCode(int code) {
        const bool success = producer_.Push(std::move(code));
        ASSERT_TRUE(success);
    }

    clients::http::Client& GetClient() { return *http_client_; }

    const utest::HttpServerMock& GetServer() { return http_server_; }

private:
    const std::shared_ptr<clients::http::Client> http_client_ = utest::CreateHttpClient();
    const std::shared_ptr<ResponseQueue> queue_ = ResponseQueue::Create();
    const utest::HttpServerMock http_server_{WaitingEchoHandler{*queue_}};
    ResponseQueue::Producer producer_ = queue_->GetProducer();
};

void SetTaskInheritedDeadline(std::chrono::milliseconds ms) {
    server::request::TaskInheritedData data;
    data.deadline = engine::Deadline::FromDuration(ms);
    server::request::kTaskInheritedData.Set(std::move(data));
}

void WaitForPeerClose(engine::io::Socket& socket, engine::Deadline deadline) {
    constexpr auto kReadBufferSize = 4096;
    std::array<char, kReadBufferSize> buffer{};
    try {
        while (socket.RecvSome(buffer.data(), buffer.size(), deadline) != 0) {
        }
    } catch (const engine::io::IoSystemError& ex) {
        if (ex.Code() != std::errc::connection_reset) {
            throw;
        }
    }
}

}  // namespace

UTEST_F(HttpClientDeadline, ZeroDeadlineIsUsedByClient) {
    SetTaskInheritedDeadline(0ms);

    auto request = GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    UEXPECT_THROW((void)request.perform(), clients::http::CancelException);
}

UTEST_F(HttpClientDeadline, DeadlineIsUsedByClient) {
    SetTaskInheritedDeadline(100ms);

    auto request = GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    UEXPECT_THROW((void)request.perform(), clients::http::CancelException);
}

UTEST_F(HttpClientDeadline, DeadlineIsUsedByClientOldConnection) {
    SetTaskInheritedDeadline(100ms);

    PushResponseCode(200);
    auto success_request =
        GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    UEXPECT_NO_THROW(EXPECT_TRUE(success_request.perform()->IsOk()));

    auto expired_request =
        GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    UEXPECT_THROW((void)expired_request.perform(), clients::http::CancelException);
}

UTEST_F(HttpClientDeadline, ConnectionIsReused) {
    for ([[maybe_unused]] const auto _ : std::views::iota(0, 3)) {
        PushResponseCode(200);

        const auto response =
            GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime).perform();
        EXPECT_TRUE(response->IsOk());
    }

    EXPECT_EQ(GetServer().GetConnectionsOpenedCount(), 1);
}

UTEST_F(HttpClientDeadline, ConnectionIsBrokenAfterTimeout) {
    constexpr auto kRequestTimeout = 10ms;
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    engine::io::tests::TcpListener listener;
    auto server = utils::Async("check-timed-out-connections", [&listener, deadline] {
        constexpr auto kConnectionsToCheck = 3;
        for ([[maybe_unused]] const auto _ : std::views::iota(0, kConnectionsToCheck)) {
            auto socket = listener.socket.Accept(deadline);
            WaitForPeerClose(socket, deadline);
        }
    });

    // Retain futures so cancellation on destruction cannot produce the EOF being tested.
    std::vector<clients::http::ResponseFuture> responses;
    const auto url = fmt::format("http://[::1]:{}/", listener.Port());
    while (!server.IsFinished() && !deadline.IsReached()) {
        auto request = GetClient().CreateRequest().get(url).timeout(kRequestTimeout);
        responses.push_back(request.async_perform());
        UASSERT_THROW(responses.back().Get(), clients::http::TimeoutException);
    }

    UEXPECT_NO_THROW(server.Get());
}

UTEST_F(HttpClientDeadline, ConnectionIsKeptAfterDeadlineExpires) {
    constexpr auto kTimeToCleanUpConnection = 100ms;

    for ([[maybe_unused]] const auto _ : std::views::iota(0, 3)) {
        SetTaskInheritedDeadline(100ms);

        auto request = GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
        UEXPECT_THROW((void)request.perform(), clients::http::CancelException);

        // Client should wait for the connection to clean up for the duration of the
        // static timeout. Server will send the response when we allow the handler
        // to complete.
        PushResponseCode(504);
        engine::SleepFor(kTimeToCleanUpConnection);
    }

    EXPECT_EQ(GetServer().GetConnectionsOpenedCount(), 1);
}

UTEST_F(HttpClientDeadline, NoCrashOnUnfinishedRequestReuse) {
    auto request = GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    for ([[maybe_unused]] const auto _ : std::views::iota(0, 3)) {
        SetTaskInheritedDeadline(100ms);

        UEXPECT_THROW((void)request.perform(), clients::http::CancelException);
        PushResponseCode(200);
    }
}

UTEST_F(HttpClientDeadline, NoCrashOnAlreadyTimedOutRequestReuse) {
    auto request = GetClient().CreateRequest().get().url(GetServer().GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
    for ([[maybe_unused]] const auto _ : std::views::iota(0, 3)) {
        SetTaskInheritedDeadline(-100ms);

        UEXPECT_THROW((void)request.perform(), clients::http::CancelException);
        PushResponseCode(200);
    }
}

USERVER_NAMESPACE_END
