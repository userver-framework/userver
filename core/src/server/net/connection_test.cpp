#include <server/net/connection.hpp>

#include <fmt/format.h>

#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/http/request_handler_base.hpp>
#include <server/net/create_socket.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/utest/http_client.hpp>
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
      : behavior_(behavior) {}

  engine::TaskWithResult<void> StartRequestTask(
      std::shared_ptr<server::request::RequestBase> request) const override {
    UASSERT(request);

    auto& http_request = dynamic_cast<server::http::HttpRequestImpl&>(*request);
    static server::handlers::HttpRequestStatistics statistics;
    http_request.SetHttpHandlerStatistics(statistics);

    switch (behavior_) {
      case Behaviors::kNoop:
        return engine::AsyncNoSpan([this]() { ++asyncs_finished; });
      case Behaviors::kHang:
        return engine::AsyncNoSpan([this]() {
          engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
          ASSERT_TRUE(engine::current_task::IsCancelRequested());
          ++asyncs_finished;
        });
    }

    UINVARIANT(false, "Unexpected behavior");
  }

  const server::http::HandlerInfoIndex& GetHandlerInfoIndex() const override {
    return handler_info_index_;
  }

  const logging::LoggerPtr& LoggerAccess() const noexcept override {
    return no_logger_;
  };
  const logging::LoggerPtr& LoggerAccessTskv() const noexcept override {
    return no_logger_;
  };

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  mutable std::atomic<std::size_t> asyncs_finished{0};

 private:
  const Behaviors behavior_;
  logging::LoggerPtr no_logger_;
  server::http::HandlerInfoIndex handler_info_index_;
};

std::string HttpConnectionUriFromSocket(engine::io::Socket& sock) {
  return fmt::format("http://localhost:{}", sock.Getsockname().Port());
}

static const utils::http::HttpVersion kDefaultHttpVer =
    utils::http::HttpVersion::k11;

enum class ConnectionHeader {
  kKeepAlive,
  kClose,
};

clients::http::ResponseFuture CreateRequest(
    clients::http::Client& http_client, engine::io::Socket& request_socket,
    utils::http::HttpVersion http_ver = utils::http::HttpVersion::k11,
    ConnectionHeader header = ConnectionHeader::kKeepAlive) {
  auto ret = http_client.CreateRequest()
                 .http_version(http_ver)
                 .get(HttpConnectionUriFromSocket(request_socket))
                 .retry(1)
                 .timeout(std::chrono::milliseconds(100));
  if (header == ConnectionHeader::kClose) {
    ret.headers({{"Connection", "close"}});
  }
  return ret.async_perform();
}

net::ListenerConfig CreateConfig(
    utils::http::HttpVersion http_ver = utils::http::HttpVersion::k11) {
  net::ListenerConfig config;
  config.handler_defaults = server::request::HttpRequestConfig{};
  config.handler_defaults.http_version = http_ver;
  return config;
}

const std::vector<utils::http::HttpVersion> protocols{
    utils::http::HttpVersion::k11, utils::http::HttpVersion::k2};

// The param tells which http protocol to use.
class ServerNetConnection
    : public testing::TestWithParam<utils::http::HttpVersion> {};

}  // namespace

INSTANTIATE_UTEST_SUITE_P(/*no prefix*/, ServerNetConnection,
                          ::testing::Values(utils::http::HttpVersion::k11,
                                            utils::http::HttpVersion::k2));

UTEST_P(ServerNetConnection, EarlyCancel) {
  const auto http_ver = GetParam();
  net::ListenerConfig config = CreateConfig(http_ver);
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  auto request = CreateRequest(*http_client_ptr, request_socket, http_ver,
                               ConnectionHeader::kKeepAlive);

  auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
  ASSERT_TRUE(peer.IsValid());
  auto stats = std::make_shared<net::Stats>();
  server::request::ResponseDataAccounter data_accounter;
  TestHttprequestHandler handler;

  auto task = engine::AsyncNoSpan([&] {
    net::Connection connection(
        config.connection_config, config.handler_defaults,
        std::make_unique<engine::io::Socket>(std::move(peer)), {}, handler,
        stats, data_accounter);

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

UTEST_P(ServerNetConnection, EarlyTimeout) {
  const auto http_ver = GetParam();
  net::ListenerConfig config = CreateConfig(http_ver);
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  auto res = CreateRequest(*http_client_ptr, request_socket, http_ver,
                           ConnectionHeader::kKeepAlive);

  engine::io::Socket peer =
      request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
  ASSERT_TRUE(peer.IsValid());
  auto stats = std::make_shared<net::Stats>();
  server::request::ResponseDataAccounter data_accounter;
  TestHttprequestHandler handler;

  UEXPECT_THROW(res.Get(), clients::http::TimeoutException);

  auto task = engine::AsyncNoSpan([&] {
    net::Connection connection(
        config.connection_config, config.handler_defaults,
        std::make_unique<engine::io::Socket>(std::move(peer)), {}, handler,
        stats, data_accounter);

    connection.Process();
  });
  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

UTEST_P(ServerNetConnection, TimeoutWithTaskCancellation) {
  const auto http_ver = GetParam();
  net::ListenerConfig config = CreateConfig(http_ver);
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  auto res = CreateRequest(*http_client_ptr, request_socket, http_ver,
                           ConnectionHeader::kKeepAlive);

  engine::io::Socket peer =
      request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
  ASSERT_TRUE(peer.IsValid());
  auto stats = std::make_shared<net::Stats>();
  server::request::ResponseDataAccounter data_accounter;
  TestHttprequestHandler handler{TestHttprequestHandler::Behaviors::kHang};

  auto task = engine::AsyncNoSpan([&] {
    net::Connection connection(
        config.connection_config, config.handler_defaults,
        std::make_unique<engine::io::Socket>(std::move(peer)), {}, handler,
        stats, data_accounter);

    connection.Process();
  });

  task.RequestCancel();
  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
  UEXPECT_THROW(res.Get(), clients::http::TimeoutException);
}

UTEST_P(ServerNetConnection, EarlyTeardown) {
  const auto http_ver = GetParam();
  net::ListenerConfig config = CreateConfig(http_ver);
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  auto res = CreateRequest(*http_client_ptr, request_socket, http_ver,
                           ConnectionHeader::kClose);

  engine::io::Socket peer =
      request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
  ASSERT_TRUE(peer.IsValid());

  UEXPECT_THROW(res.Get(), clients::http::TimeoutException);
  res.Cancel();
  peer.Close();
  request_socket.Close();
  http_client_ptr.reset();
}

UTEST_P(ServerNetConnection, RemoteClosed) {
  net::ListenerConfig config = CreateConfig();
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  auto request = CreateRequest(*http_client_ptr, request_socket,
                               kDefaultHttpVer, ConnectionHeader::kClose);

  auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
  ASSERT_TRUE(peer.IsValid());
  auto stats = std::make_shared<net::Stats>();
  server::request::ResponseDataAccounter data_accounter;
  TestHttprequestHandler handler;

  auto task = engine::AsyncNoSpan([&] {
    net::Connection connection(
        config.connection_config, config.handler_defaults,
        std::make_unique<engine::io::Socket>(std::move(peer)), {}, handler,
        stats, data_accounter);

    connection.Process();
  });
  EXPECT_EQ(request.Get()->status_code(), 404);

  task.RequestCancel();
  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

UTEST_P(ServerNetConnection, KeepAlive) {
  net::ListenerConfig config = CreateConfig();
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  http_client_ptr->SetMaxHostConnections(1);

  auto request = CreateRequest(*http_client_ptr, request_socket,
                               kDefaultHttpVer, ConnectionHeader::kKeepAlive);

  auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
  ASSERT_TRUE(peer.IsValid());
  auto stats = std::make_shared<net::Stats>();
  server::request::ResponseDataAccounter data_accounter;
  TestHttprequestHandler handler;

  auto task = engine::AsyncNoSpan([&] {
    net::Connection connection(
        config.connection_config, config.handler_defaults,
        std::make_unique<engine::io::Socket>(std::move(peer)), {}, handler,
        stats, data_accounter);

    connection.Process();
  });
  EXPECT_EQ(request.Get()->status_code(), 404);

  EXPECT_EQ(handler.asyncs_finished, 1);
  request = CreateRequest(*http_client_ptr, request_socket, kDefaultHttpVer,
                          ConnectionHeader::kKeepAlive);
  EXPECT_EQ(request.Get()->status_code(), 404);
  EXPECT_EQ(handler.asyncs_finished, 2);
}

UTEST_P(ServerNetConnection, CancelMultipleInFlight) {
  constexpr std::size_t kInFlightRequests = 10;
  constexpr std::size_t kMaxAttempts = 10;
  net::ListenerConfig config = CreateConfig();
  auto request_socket = net::CreateSocket(config);

  auto http_client_ptr = utest::CreateHttpClient();
  http_client_ptr->SetMaxHostConnections(1);

  for (unsigned ii = 0; ii < kMaxAttempts; ++ii) {
    auto res = CreateRequest(*http_client_ptr, request_socket);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    auto stats = std::make_shared<net::Stats>();
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto task = engine::AsyncNoSpan([&] {
      net::Connection connection(
          config.connection_config, config.handler_defaults,
          std::make_unique<engine::io::Socket>(std::move(peer)), {}, handler,
          stats, data_accounter);

      connection.Process();
    });
    res.Wait();
    EXPECT_EQ(handler.asyncs_finished, 1);
    handler.asyncs_finished = 0;

    task.RequestCancel();
    ASSERT_TRUE(!task.IsFinished());  // keep-alive should work

    for (unsigned i = 0; i < kInFlightRequests; ++i) {
      CreateRequest(*http_client_ptr, request_socket).Detach();
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

USERVER_NAMESPACE_END
