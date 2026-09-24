#include <server/net/http1_connection.hpp>
#include <server/net/http2_connection.hpp>

#include <cerrno>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_response_impl.hpp>
#include <server/http/request_handler_base.hpp>
#include <server/net/create_socket.hpp>
#include <server/request/response_data_accounter.hpp>
#include <userver/clients/http/client_core.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_log_context.hpp>

#include <userver/utest/http_client.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace net = server::net;
using engine::Deadline;

namespace {
constexpr auto kAcceptTimeout = utest::kMaxTestWaitTime;

class TestHttprequestHandler : public server::http::RequestHandlerBase {
public:
    enum class Behaviors { kNoop, kHang };

    explicit TestHttprequestHandler(Behaviors behavior = Behaviors::kNoop)
        : behavior_(behavior)
    {}

    engine::TaskWithResult<void> StartRequestTask(std::shared_ptr<server::http::HttpRequest> http_request
    ) const override {
        UASSERT(http_request);

        switch (behavior_) {
            case Behaviors::kNoop:
                return engine::AsyncNoTracing([this]() { ++asyncs_finished; });
            case Behaviors::kHang:
                return engine::AsyncNoTracing([this]() {
                    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
                    ASSERT_TRUE(engine::current_task::IsCancelRequested());
                    ++asyncs_finished;
                });
        }

        UINVARIANT(false, "Unexpected behavior");
    }

    const server::http::HandlerInfoIndex& GetHandlerInfoIndex() const override { return handler_info_index_; }

    const logging::TextLoggerPtr& LoggerAccess() const noexcept override { return no_logger_; };
    const logging::TextLoggerPtr& LoggerAccessTskv() const noexcept override { return no_logger_; };

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    mutable std::atomic<std::size_t> asyncs_finished{0};

private:
    const Behaviors behavior_;
    logging::TextLoggerPtr no_logger_;
    server::http::HandlerInfoIndex handler_info_index_;
};

std::string HttpConnectionUriFromSocket(engine::io::Socket& sock) {
    return fmt::format("http://localhost:{}", sock.Getsockname().Port());
}

enum class ConnectionHeader {
    kKeepAlive,
    kClose,
};

// Short timeout for the tests that deliberately expect a `TimeoutException`
// (the server never reads the request in those cases).
constexpr auto kFailureRequestTimeout = std::chrono::milliseconds(100);
// Generous timeout for the tests that expect the request to actually succeed.
// A tight timeout here flaps under sanitizers (MSAN/TSAN), where instrumentation
// slows down the accept -> spawn connection task -> parse -> respond chain (the
// HTTP/2 SETTINGS handshake adds an extra round-trip) well past 100ms.
constexpr auto kSuccessRequestTimeout = utest::kMaxTestWaitTime;

clients::http::ResponseFuture CreateRequest(
    clients::http::Client& http_client,
    engine::io::Socket& request_socket,
    std::chrono::milliseconds timeout,
    USERVER_NAMESPACE::http::HttpVersion http_ver = USERVER_NAMESPACE::http::HttpVersion::k11,
    ConnectionHeader header = ConnectionHeader::kKeepAlive
) {
    auto ret =
        http_client.CreateRequest()
            .http_version(http_ver)
            .get(HttpConnectionUriFromSocket(request_socket))
            .retry(1)
            .timeout(timeout);
    if (header == ConnectionHeader::kClose) {
        ret.headers({{"Connection", "close"}});
    }
    return ret.async_perform();
}

net::ListenerConfig CreateConfig(
    USERVER_NAMESPACE::http::HttpVersion http_ver = USERVER_NAMESPACE::http::HttpVersion::k11
) {
    net::ListenerConfig config;
    config.handler_defaults = server::request::HttpRequestConfig{};
    config.connection_config.http_version = http_ver;

    net::PortConfig port_config;
    port_config.port = 32000;
    config.ports.emplace_back(std::move(port_config));
    return config;
}

template <typename ConnectionType>
USERVER_NAMESPACE::http::HttpVersion HttpVersion() {
    if constexpr (std::is_same_v<ConnectionType, net::Http2Connection>) {
        return USERVER_NAMESPACE::http::HttpVersion::k2;
    }
    return USERVER_NAMESPACE::http::HttpVersion::k11;
}

// The param tells which http protocol to use.
template <typename T>
class ServerNetConnection : public ::testing::Test {};

using ConnectionTypes = ::testing::Types<net::Http1Connection, net::Http2Connection>;

constexpr char kResponseWriteError[] = "response write failed";

class FailingResponseSocket final : public engine::io::RwBase {
public:
    MOCK_METHOD(bool, IsValid, (), (const, override));
    MOCK_METHOD(bool, WaitReadable, (Deadline), (override));
    MOCK_METHOD(bool, WaitWriteable, (Deadline), (override));
    MOCK_METHOD(std::size_t, ReadSome, (void*, std::size_t, Deadline), (override));
    MOCK_METHOD(std::size_t, ReadAll, (void*, std::size_t, Deadline), (override));
    MOCK_METHOD(std::size_t, WriteAll, (const void*, std::size_t, Deadline), (override));
};

class TracedResponseHandler final : public TestHttprequestHandler {
public:
    explicit TracedResponseHandler(bool enable_tracing)
        : enable_tracing_(enable_tracing)
    {}

    engine::TaskWithResult<void> StartRequestTask(std::shared_ptr<server::http::HttpRequest> request) const override {
        return engine::AsyncNoTracing([request = std::move(request), enable_tracing = enable_tracing_] {
            tracing::SpanLogContext expected_context;
            auto& response = server::http::GetHttpResponseImpl(*request);
            if (enable_tracing) {
                const tracing::Span handler_span{"response-handler"};
                response.SetTracingContext(handler_span);
                expected_context.SetFromSpan(handler_span);
            }
            ASSERT_EQ(response.GetTracingContext(), expected_context);
            LOG_INFO() << "response prepared" << expected_context.GetLogExtra();
            response.SetReady();
        });
    }

private:
    const bool enable_tracing_;
};

using ServerResponseSendError = utest::LogCaptureFixture<testing::TestWithParam<std::tuple<bool, int>>>;

}  // namespace

UTEST_P(ServerResponseSendError, TracingContext) {
    const auto [enable_tracing, error_code] = GetParam();
    GetLogCapture().Clear();
    auto socket = std::make_unique<FailingResponseSocket>();
    EXPECT_CALL(*socket, WriteAll(testing::_, testing::_, testing::_))
        .WillOnce([error_code](const void*, std::size_t, Deadline) -> std::size_t {
            if (error_code != 0) {
                throw engine::io::IoSystemError(error_code, kResponseWriteError);
            }
            throw std::runtime_error(kResponseWriteError);
        });

    const net::ConnectionConfig config{
        .abort_check_delay = utest::kMaxTestWaitTime,
        .http2_session_config = {},
    };
    const server::request::HttpRequestConfig request_config;
    net::Stats stats{};
    server::request::ResponseDataAccounter accounter;
    const TracedResponseHandler handler{enable_tracing};
    net::Http1Connection connection{config, request_config, std::move(socket), {}, handler, stats, accounter};
    auto request = server::http::HttpRequestBuilder{accounter}.Build();
    engine::AsyncNoTracing([&connection, request = std::move(request)]() mutable {
        connection.ProcessRequest(std::move(request));
    }).Get();

    const auto prepared = utest::GetSingleLog(GetLogCapture().Filter("response prepared"));
    const auto failed = utest::GetSingleLog(GetLogCapture().Filter(kResponseWriteError));
    for (const auto tag : {"trace_id", "span_id", "link", "trace_sampled"}) {
        SCOPED_TRACE(tag);
        const auto expected_tag = prepared.GetTagOptional(tag);
        ASSERT_EQ(expected_tag.has_value(), enable_tracing);
        EXPECT_EQ(failed.GetTagOptional(tag), expected_tag);
    }
    EXPECT_EQ(failed.GetLevel(), error_code == EPIPE ? logging::Level::kWarning : logging::Level::kError);
}

INSTANTIATE_UTEST_SUITE_P(
    TracingAndWriteErrors,
    ServerResponseSendError,
    testing::Combine(testing::Bool(), testing::Values(ECONNRESET, EPIPE, 0))
);

TYPED_UTEST_SUITE(ServerNetConnection, ConnectionTypes);

TYPED_UTEST(ServerNetConnection, EarlyCancel) {
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::CreateHttpClient();
    auto request =
        CreateRequest(*http_client_ptr, request_socket, kFailureRequestTimeout, http_ver, ConnectionHeader::kKeepAlive);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        connection.Process();
    });

    // Immediately canceling the `socket_listener_` task without giving it
    // any chance to start.
    task.RequestCancel();
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
    UEXPECT_THROW(request.Get(), std::exception)
        << "Looks like the `socket_listener_` task was started (the "
           "request "
           "was received and processed). Too bad: the test tested nothing";
}

TYPED_UTEST(ServerNetConnection, EarlyTimeout) {
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::CreateHttpClient();
    auto res =
        CreateRequest(*http_client_ptr, request_socket, kFailureRequestTimeout, http_ver, ConnectionHeader::kKeepAlive);

    engine::io::Socket peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    UEXPECT_THROW(res.Get(), clients::http::TimeoutException);

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        connection.Process();
    });
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
}

TYPED_UTEST(ServerNetConnection, TimeoutWithTaskCancellation) {
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::CreateHttpClient();
    auto res =
        CreateRequest(*http_client_ptr, request_socket, kFailureRequestTimeout, http_ver, ConnectionHeader::kKeepAlive);

    engine::io::Socket peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler{TestHttprequestHandler::Behaviors::kHang};

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        connection.Process();
    });

    task.RequestCancel();
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
    UEXPECT_THROW(res.Get(), clients::http::TimeoutException);
}

TYPED_UTEST(ServerNetConnection, EarlyTeardown) {
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::CreateHttpClient();
    auto res =
        CreateRequest(*http_client_ptr, request_socket, kFailureRequestTimeout, http_ver, ConnectionHeader::kClose);

    engine::io::Socket peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());

    UEXPECT_THROW(res.Get(), clients::http::TimeoutException);
    res.Cancel();
    peer.Close();
    request_socket.Close();
    http_client_ptr.reset();
}

TYPED_UTEST(ServerNetConnection, RemoteClosed) {
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::CreateHttpClient();
    auto request =
        CreateRequest(*http_client_ptr, request_socket, kSuccessRequestTimeout, http_ver, ConnectionHeader::kClose);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        connection.Process();
    });
    EXPECT_EQ(request.Get()->status_code(), 404);

    task.RequestCancel();
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
}

TYPED_UTEST(ServerNetConnection, KeepAlive) {
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::impl::CreateHttpClientCore();
    http_client_ptr->SetMaxHostConnections(1);

    auto request =
        CreateRequest(*http_client_ptr, request_socket, kSuccessRequestTimeout, http_ver, ConnectionHeader::kKeepAlive);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        connection.Process();
    });
    EXPECT_EQ(request.Get()->status_code(), 404);

    EXPECT_EQ(handler.asyncs_finished, 1);
    request =
        CreateRequest(*http_client_ptr, request_socket, kSuccessRequestTimeout, http_ver, ConnectionHeader::kKeepAlive);
    EXPECT_EQ(request.Get()->status_code(), 404);
    EXPECT_EQ(handler.asyncs_finished, 2);
}

TYPED_UTEST(ServerNetConnection, IdleKeepAliveTimeout) {
    // After a completed keep-alive request the connection goes idle. The server
    // must close it on `keepalive_timeout` by itself (no cancellation).
    using ConnectionType = TypeParam;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    config.connection_config.keepalive_timeout = std::chrono::seconds{1};
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::CreateHttpClient();
    auto request =
        CreateRequest(*http_client_ptr, request_socket, kSuccessRequestTimeout, http_ver, ConnectionHeader::kKeepAlive);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        connection.Process();
    });
    EXPECT_EQ(request.Get()->status_code(), 404);

    // The connection is idle now; it must terminate on the keepalive timeout
    // with nobody cancelling the task.
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
}

TYPED_UTEST(ServerNetConnection, CancelMultipleInFlight) {
    using ConnectionType = TypeParam;
    constexpr std::size_t kInFlightRequests = 10;
    constexpr std::size_t kMaxAttempts = 10;
    const auto http_ver = HttpVersion<ConnectionType>();
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::impl::CreateHttpClientCore();
    http_client_ptr->SetMaxHostConnections(1);

    for (unsigned ii = 0; ii < kMaxAttempts; ++ii) {
        auto res = CreateRequest(
            *http_client_ptr,
            request_socket,
            kSuccessRequestTimeout,
            http_ver,
            ConnectionHeader::kKeepAlive
        );

        auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
        ASSERT_TRUE(peer.IsValid());
        net::Stats stats{};
        server::request::ResponseDataAccounter data_accounter;
        TestHttprequestHandler handler;

        auto task = engine::AsyncNoTracing([&] {
            ConnectionType connection(
                config.connection_config,
                config.handler_defaults,
                std::make_unique<engine::io::Socket>(std::move(peer)),
                {},
                handler,
                stats,
                data_accounter
            );

            connection.Process();
        });
        res.Wait();
        EXPECT_EQ(handler.asyncs_finished, 1);
        handler.asyncs_finished = 0;

        task.RequestCancel();
        ASSERT_TRUE(!task.IsFinished());  // keep-alive should work

        for (unsigned i = 0; i < kInFlightRequests; ++i) {
            CreateRequest(*http_client_ptr, request_socket, kFailureRequestTimeout, http_ver).Detach();
        }

        task.RequestCancel();
        task.WaitFor(utest::kMaxTestWaitTime / kMaxAttempts);

        if (handler.asyncs_finished < kInFlightRequests) {
            return;  // success, requests were cancelled
        }
    }

    // Note: comment out the next line in case of flaps
    FAIL() << "Failed to simulate cancellation of multiple requests";
}

UTEST(HTTP2Connection, ThrowHttp1IsNotSupported) {
    using ConnectionType = server::net::Http2Connection;
    const auto http_ver = USERVER_NAMESPACE::http::HttpVersion::k2;
    net::ListenerConfig config = CreateConfig(http_ver);
    auto request_socket = net::CreateSocket(config, config.ports[0]);

    auto http_client_ptr = utest::impl::CreateHttpClientCore();
    http_client_ptr->SetMaxHostConnections(1);

    [[maybe_unused]] auto request = CreateRequest(
        *http_client_ptr,
        request_socket,
        kFailureRequestTimeout,
        USERVER_NAMESPACE::http::HttpVersion::k11,
        ConnectionHeader::kKeepAlive
    );

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    net::Stats stats{};
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto task = engine::AsyncNoTracing([&] {
        ConnectionType connection(
            config.connection_config,
            config.handler_defaults,
            std::make_unique<engine::io::Socket>(std::move(peer)),
            {},
            handler,
            stats,
            data_accounter
        );

        UASSERT_THROW(connection.Process(), server::net::Http1IsNotSupported);
    });
    task.Get();
}

USERVER_NAMESPACE_END
