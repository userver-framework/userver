#include "tcp_socket_sink.hpp"

#include <boost/regex.hpp>

#include <userver/internal/net/net_listener.hpp>
#include <userver/net/blocking/get_addr_info.hpp>

#include <userver/engine/async.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/text.hpp>

#include "sink_helper_test.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(TcpSocketSink, SocketConnectErrorLocalhost) {
  auto addrs = net::blocking::GetAddrInfo("localhost", "8111");
  auto socket_sink = logging::impl::TcpSocketSink(addrs);
  EXPECT_THROW(socket_sink.Log({"msg", logging::Level::kWarning}),
               engine::io::IoSystemError);
}

UTEST(TcpSocketSink, SocketConnectErrorIpV6) {
  auto addrs = net::blocking::GetAddrInfo("::1", "8222");
  auto socket_sink = logging::impl::TcpSocketSink(addrs);
  EXPECT_THROW(socket_sink.Log({"msg", logging::Level::kWarning}),
               engine::io::IoSystemError);
}

UTEST(TcpSocketSink, SocketConnectErrorIpV4) {
  auto addrs = net::blocking::GetAddrInfo("127.0.0.1", "8333");
  auto socket_sink = logging::impl::TcpSocketSink(addrs);
  EXPECT_THROW(socket_sink.Log({"msg", logging::Level::kWarning}),
               engine::io::IoSystemError);
}

UTEST(TcpSocketSink, SocketConnectV4) {
  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});
  EXPECT_NO_THROW(socket_sink.Log({"msg", logging::Level::kWarning}));
}

UTEST(TcpSocketSink, SocketConnectV6) {
  internal::net::TcpListener listener(internal::net::IpVersion::kV6);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});
  EXPECT_NO_THROW(socket_sink.Log({"msg", logging::Level::kWarning}));
}

UTEST(TcpSocketSink, SinkReadOnceV4) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});

  auto listen_task = engine::AsyncNoSpan([&listener, &deadline] {
    auto sock = listener.socket.Accept(deadline);
    const auto data = test::ReadFromSocket(std::move(sock));
    ASSERT_EQ(data.size(), 1);
    EXPECT_EQ(data[0], "message");
  });

  EXPECT_NO_THROW(socket_sink.Log({"message\n", logging::Level::kWarning}));

  socket_sink.Close();
  listen_task.Get();
}

UTEST(TcpSocketSink, SinkReadMoreV4) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});

  auto listen_task = engine::AsyncNoSpan([&listener, &deadline] {
    auto sock = listener.socket.Accept(deadline);
    const auto logs = test::ReadFromSocket(std::move(sock));
    ASSERT_EQ(logs.size(), 3);
    EXPECT_EQ(logs[0], "message");
    EXPECT_EQ(logs[1], "message 2");
    EXPECT_EQ(logs[2], "message 3");
  });

  EXPECT_NO_THROW(socket_sink.Log({"message\n", logging::Level::kWarning}));
  EXPECT_NO_THROW(socket_sink.Log({"message 2\n", logging::Level::kInfo}));
  EXPECT_NO_THROW(socket_sink.Log({"message 3\n", logging::Level::kCritical}));
  socket_sink.Close();
  listen_task.Get();
}

UTEST_MT(TcpSocketSink, ConcurrentClose, 4) {
  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});

  auto log_task_1 = engine::AsyncNoSpan(
      [&socket_sink] { EXPECT_NO_THROW(socket_sink.Close()); });

  auto log_task_2 = engine::AsyncNoSpan(
      [&socket_sink] { EXPECT_NO_THROW(socket_sink.Close()); });

  auto log_task_3 = engine::AsyncNoSpan(
      [&socket_sink] { EXPECT_NO_THROW(socket_sink.Close()); });

  log_task_1.Get();
  log_task_2.Get();
  log_task_3.Get();
}

USERVER_NAMESPACE_END
