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
  EXPECT_THROW(socket_sink.log({"default", spdlog::level::warn, "msg"}),
               engine::io::IoSystemError);
}

UTEST(TcpSocketSink, SocketConnectErrorIpV6) {
  auto addrs = net::blocking::GetAddrInfo("::1", "8222");
  auto socket_sink = logging::impl::TcpSocketSink(addrs);
  EXPECT_THROW(socket_sink.log({"default", spdlog::level::warn, "msg"}),
               engine::io::IoSystemError);
}

UTEST(TcpSocketSink, SocketConnectErrorIpV4) {
  auto addrs = net::blocking::GetAddrInfo("127.0.0.1", "8333");
  auto socket_sink = logging::impl::TcpSocketSink(addrs);
  EXPECT_THROW(socket_sink.log({"default", spdlog::level::warn, "msg"}),
               engine::io::IoSystemError);
}

UTEST(TcpSocketSink, SocketConnectV4) {
  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});
  EXPECT_NO_THROW(socket_sink.log({"default", spdlog::level::warn, "msg"}));
}

UTEST(TcpSocketSink, SocketConnectV6) {
  internal::net::TcpListener listener(internal::net::IpVersion::kV6);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});
  EXPECT_NO_THROW(socket_sink.log({"default", spdlog::level::warn, "msg"}));
}

UTEST(TcpSocketSink, SinkReadOnceV4) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});

  auto listen_task = engine::AsyncNoSpan([&listener, &deadline] {
    auto sock = listener.socket.Accept(deadline);
    const auto data = test::ReadFromSocket(std::move(sock));
    EXPECT_EQ(data[0], "[datetime] [default] [warning] message");
  });

  EXPECT_NO_THROW(socket_sink.log({"default", spdlog::level::warn, "message"}));

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
    EXPECT_EQ(logs[0], "[datetime] [default] [warning] message");
    EXPECT_EQ(logs[1], "[datetime] [basic] [info] message 2");
    EXPECT_EQ(logs[2], "[datetime] [current] [critical] message 3");
  });

  EXPECT_NO_THROW(socket_sink.log({"default", spdlog::level::warn, "message"}));
  EXPECT_NO_THROW(socket_sink.log({"basic", spdlog::level::info, "message 2"}));
  EXPECT_NO_THROW(
      socket_sink.log({"current", spdlog::level::critical, "message 3"}));
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
