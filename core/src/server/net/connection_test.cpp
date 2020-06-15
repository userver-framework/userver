#include <server/net/connection.hpp>

#include <fmt/format.h>

#include <clients/http/client.hpp>
#include <engine/io/addr.hpp>
#include <engine/sleep.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/http/request_handler_base.hpp>
#include <server/net/create_socket.hpp>

#include <utest/http_client.hpp>
#include <utest/utest.hpp>

namespace net = server::net;
using engine::Deadline;

namespace {
constexpr std::chrono::seconds kAcceptTimeout{1};

class TestHttprequestHandler : public server::http::RequestHandlerBase {
 public:
  enum class Behaviors { kNoop, kHang };

  explicit TestHttprequestHandler(Behaviors behavior = Behaviors::kNoop)
      : behavior_(behavior) {}

  engine::TaskWithResult<void> StartRequestTask(
      std::shared_ptr<server::request::RequestBase> request) const override {
    UASSERT(request);

    auto& http_request = dynamic_cast<server::http::HttpRequestImpl&>(*request);
    static server::handlers::HttpHandlerStatistics statistics;
    http_request.SetHttpHandlerStatistics(statistics);

    switch (behavior_) {
      case Behaviors::kNoop:
        return engine::impl::Async([this]() { ++asyncs_finished; });
      case Behaviors::kHang:
        return engine::impl::Async([this]() {
          engine::InterruptibleSleepFor(kMaxTestWaitTime);
          ASSERT_TRUE(engine::current_task::IsCancelRequested());
          ++asyncs_finished;
        });
    }
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

  mutable std::atomic<std::size_t> asyncs_finished{0};

 private:
  const Behaviors behavior_;
  logging::LoggerPtr no_logger_;
  server::http::HandlerInfoIndex handler_info_index_;
};

std::string HttpConnectionUriFromSocket(engine::io::Socket& sock) {
  return fmt::format("http://localhost:{}",
                     engine::io::GetPort(sock.Getsockname()));
}

enum class ConnectionHeader {
  kKeepAlive,
  kClose,
};

clients::http::ResponseFuture CreateRequest(
    std::shared_ptr<clients::http::Client> http_client_ptr,
    engine::io::Socket& request_socket,
    ConnectionHeader header = ConnectionHeader::kKeepAlive) {
  auto ret = http_client_ptr->CreateRequest()
                 ->get(HttpConnectionUriFromSocket(request_socket))
                 ->retry(1)
                 ->timeout(std::chrono::milliseconds(100));
  if (header == ConnectionHeader::kClose) {
    ret->headers({{"Connection", "close"}});
  }
  return ret->async_perform();
}

net::ListenerConfig CreateConfig() {
  net::ListenerConfig config;
  config.connection_config.request =
      std::make_unique<server::request::RequestConfig>(
          formats::yaml::Value{}, std::string{}, yaml_config::VariableMapPtr{});
  return config;
}

}  // namespace

TEST(ServerNetConnection, EarlyCancel) {
  RunInCoro(
      []() {
        net::ListenerConfig config = CreateConfig();
        auto request_socket = net::CreateSocket(config);

        auto http_client_ptr = utest::CreateHttpClient();
        auto request = CreateRequest(http_client_ptr, request_socket,
                                     ConnectionHeader::kKeepAlive);

        auto peer =
            request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
        ASSERT_TRUE(peer.IsValid());
        auto stats = std::make_shared<net::Stats>();
        server::request::ResponseDataAccounter data_accounter;
        TestHttprequestHandler handler;

        auto connection_ptr = net::Connection::Create(
            engine::current_task::GetTaskProcessor(), config.connection_config,
            std::move(peer), handler, stats, data_accounter);

        connection_ptr->Start();
        // Immediately canceling the `socket_listener_` task without giving it
        // any chance to start.
        connection_ptr->Stop();
        std::weak_ptr<net::Connection> weak = connection_ptr;
        connection_ptr.reset();

        auto task = engine::impl::Async([weak]() {
          while (weak.lock()) engine::Yield();
        });

        task.WaitFor(kMaxTestWaitTime);
        EXPECT_TRUE(task.IsFinished());
        EXPECT_ANY_THROW(request.Get())
            << "Looks like the `socket_listener_` task was started (the "
               "request "
               "was received and processed). Too bad: the test tested nothing";
      },
      1);
}

TEST(ServerNetConnection, EarlyTimeout) {
  RunInCoro([]() {
    net::ListenerConfig config = CreateConfig();
    auto request_socket = net::CreateSocket(config);

    auto http_client_ptr = utest::CreateHttpClient();
    auto res = CreateRequest(http_client_ptr, request_socket,
                             ConnectionHeader::kKeepAlive);

    engine::io::Socket peer =
        request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    auto stats = std::make_shared<net::Stats>();
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    EXPECT_THROW(res.Get(), clients::http::TimeoutException);

    auto connection_ptr = net::Connection::Create(
        engine::current_task::GetTaskProcessor(), config.connection_config,
        std::move(peer), handler, stats, data_accounter);

    connection_ptr->Start();
    std::weak_ptr<net::Connection> weak = connection_ptr;
    connection_ptr.reset();

    auto task = engine::impl::Async([weak]() {
      while (weak.lock()) engine::Yield();
    });

    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(ServerNetConnection, TimeoutWithTaskCancellation) {
  RunInCoro([]() {
    net::ListenerConfig config = CreateConfig();
    auto request_socket = net::CreateSocket(config);

    auto http_client_ptr = utest::CreateHttpClient();
    auto res = CreateRequest(http_client_ptr, request_socket,
                             ConnectionHeader::kKeepAlive);

    engine::io::Socket peer =
        request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    auto stats = std::make_shared<net::Stats>();
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler{TestHttprequestHandler::Behaviors::kHang};

    auto connection_ptr = net::Connection::Create(
        engine::current_task::GetTaskProcessor(), config.connection_config,
        std::move(peer), handler, stats, data_accounter);

    connection_ptr->Start();
    std::weak_ptr<net::Connection> weak = connection_ptr;
    connection_ptr.reset();

    auto task = engine::impl::Async([weak]() {
      while (weak.lock()) engine::Yield();
    });

    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
    EXPECT_THROW(res.Get(), clients::http::TimeoutException);
  });
}

TEST(ServerNetConnection, EarlyTeardown) {
  RunInCoro([]() {
    net::ListenerConfig config = CreateConfig();
    auto request_socket = net::CreateSocket(config);

    auto http_client_ptr = utest::CreateHttpClient();
    auto res = CreateRequest(http_client_ptr, request_socket,
                             ConnectionHeader::kClose);

    engine::io::Socket peer =
        request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());

    EXPECT_THROW(res.Get(), clients::http::TimeoutException);
    peer.Close();
    request_socket.Close();

    // TODO: TAXICOMMON-1958 - uncomment and deal with hangup:
    // http_client_ptr.reset(); // tear-down
  });
}

TEST(ServerNetConnection, RemoteClosed) {
  RunInCoro([]() {
    net::ListenerConfig config = CreateConfig();
    auto request_socket = net::CreateSocket(config);

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = CreateRequest(http_client_ptr, request_socket,
                                 ConnectionHeader::kClose);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    auto stats = std::make_shared<net::Stats>();
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto connection_ptr = net::Connection::Create(
        engine::current_task::GetTaskProcessor(), config.connection_config,
        std::move(peer), handler, stats, data_accounter);

    connection_ptr->Start();
    std::weak_ptr<net::Connection> weak = connection_ptr;
    connection_ptr.reset();

    auto task = engine::impl::Async([weak]() {
      while (weak.lock()) engine::Yield();
    });

    task.WaitFor(kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(request.Get()->status_code(), 404);
  });
}

TEST(ServerNetConnection, KeepAlive) {
  RunInCoro([]() {
    net::ListenerConfig config = CreateConfig();
    auto request_socket = net::CreateSocket(config);

    auto http_client_ptr = utest::CreateHttpClient();
    http_client_ptr->SetMaxHostConnections(1);
    http_client_ptr->SetConnectionPoolSize(1);

    auto request = CreateRequest(http_client_ptr, request_socket,
                                 ConnectionHeader::kKeepAlive);

    auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
    ASSERT_TRUE(peer.IsValid());
    auto stats = std::make_shared<net::Stats>();
    server::request::ResponseDataAccounter data_accounter;
    TestHttprequestHandler handler;

    auto connection_ptr = net::Connection::Create(
        engine::current_task::GetTaskProcessor(), config.connection_config,
        std::move(peer), handler, stats, data_accounter);

    connection_ptr->Start();
    EXPECT_EQ(request.Get()->status_code(), 404);

    EXPECT_EQ(handler.asyncs_finished, 1);
    request = CreateRequest(http_client_ptr, request_socket,
                            ConnectionHeader::kKeepAlive);
    EXPECT_EQ(request.Get()->status_code(), 404);
    EXPECT_EQ(handler.asyncs_finished, 2);
  });
}

TEST(ServerNetConnection, CancelMultipleInFlight) {
  RunInCoro([]() {
    constexpr std::size_t kInFlightRequests = 10;
    constexpr std::size_t kMaxAttempts = 10;
    net::ListenerConfig config = CreateConfig();
    auto request_socket = net::CreateSocket(config);

    auto http_client_ptr = utest::CreateHttpClient();
    http_client_ptr->SetMaxHostConnections(1);
    http_client_ptr->SetConnectionPoolSize(1);

    for (unsigned ii = 0; ii < kMaxAttempts; ++ii) {
      auto res = CreateRequest(http_client_ptr, request_socket);

      auto peer = request_socket.Accept(Deadline::FromDuration(kAcceptTimeout));
      ASSERT_TRUE(peer.IsValid());
      auto stats = std::make_shared<net::Stats>();
      server::request::ResponseDataAccounter data_accounter;
      TestHttprequestHandler handler;

      auto connection_ptr = net::Connection::Create(
          engine::current_task::GetTaskProcessor(), config.connection_config,
          std::move(peer), handler, stats, data_accounter);

      connection_ptr->Start();
      res.Wait();
      EXPECT_EQ(handler.asyncs_finished, 1);
      handler.asyncs_finished = 0;

      std::weak_ptr<net::Connection> weak = connection_ptr;
      connection_ptr.reset();

      connection_ptr = weak.lock();
      ASSERT_TRUE(connection_ptr);  // keep-alive should work

      for (unsigned i = 0; i < kInFlightRequests; ++i) {
        CreateRequest(http_client_ptr, request_socket).Detach();
      }

      connection_ptr->Stop();
      connection_ptr.reset();

      auto task = engine::impl::Async([weak]() {
        while (weak.lock()) engine::Yield();
      });

      task.WaitFor(kMaxTestWaitTime / kMaxAttempts);
      EXPECT_TRUE(task.IsFinished());

      if (handler.asyncs_finished < kInFlightRequests) {
        return;  // success, requests were cancelled
      }
    }

    // Note: comment out the next line in case of flaps
    FAIL() << "Failed to simulate cancellation of multiple requests";
  });
}
