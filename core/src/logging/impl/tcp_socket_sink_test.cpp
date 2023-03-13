#include "tcp_socket_sink.hpp"

#include <userver/internal/net/net_listener.hpp>
#include <userver/net/blocking/get_addr_info.hpp>

#include <userver/engine/async.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/text.hpp>

#include <boost/regex.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const boost::regex regexp_pattern{
    R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3}\])"};
const std::string result_pattern{"[DATETIME]"};
}  // namespace

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

  auto read_sock = engine::io::Socket(listener.addr.Domain(),
                                      engine::io::SocketType::kStream);

  auto listen_task = engine::AsyncNoSpan([&listener, &deadline] {
    char buf[1];
    std::string data{};
    auto sock = listener.socket.Accept(deadline);
    while (size_t br = sock.RecvSome(buf, sizeof(buf), deadline) > 0) {
      data.push_back(buf[0]);
    }
    auto data_expect =
        boost::regex_replace(data, regexp_pattern, result_pattern,
                             boost::match_default | boost::format_sed);
    EXPECT_EQ(utils::text::ToLower(data_expect),
              utils::text::ToLower("[DATETIME] [default] [WARNING] message\n"));
  });

  EXPECT_NO_THROW(socket_sink.log({"default", spdlog::level::warn, "message"}));

  socket_sink.Close();
  listen_task.Get();
}

UTEST(TcpSocketSink, SinkReadMoreV4) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  internal::net::TcpListener listener(internal::net::IpVersion::kV4);
  auto socket_sink = logging::impl::TcpSocketSink({listener.addr});

  auto read_sock = engine::io::Socket(listener.addr.Domain(),
                                      engine::io::SocketType::kStream);

  auto listen_task = engine::AsyncNoSpan([&listener, &deadline] {
    char buf[1];
    std::string data{};
    auto sock = listener.socket.Accept(deadline);
    while (size_t br = sock.RecvSome(buf, sizeof(buf), deadline) > 0) {
      data.push_back(buf[0]);
    }
    auto logs = utils::text::Split(data, "\n");
    auto log_expect_1 =
        boost::regex_replace(logs[0], regexp_pattern, result_pattern,
                             boost::match_default | boost::format_sed);
    auto log_expect_2 =
        boost::regex_replace(logs[1], regexp_pattern, result_pattern,
                             boost::match_default | boost::format_sed);
    auto log_expect_3 =
        boost::regex_replace(logs[2], regexp_pattern, result_pattern,
                             boost::match_default | boost::format_sed);
    EXPECT_EQ(utils::text::ToLower(log_expect_1),
              utils::text::ToLower("[DATETIME] [default] [WARNING] message"));
    EXPECT_EQ(utils::text::ToLower(log_expect_2),
              utils::text::ToLower("[DATETIME] [basic] [INFO] message 2"));
    EXPECT_EQ(
        utils::text::ToLower(log_expect_3),
        utils::text::ToLower("[DATETIME] [current] [CRITICAL] message 3"));
  });

  EXPECT_NO_THROW(socket_sink.log({"default", spdlog::level::warn, "message"}));
  EXPECT_NO_THROW(socket_sink.log({"basic", spdlog::level::info, "message 2"}));
  EXPECT_NO_THROW(
      socket_sink.log({"current", spdlog::level::critical, "message 3"}));
  socket_sink.Close();
  listen_task.Get();
}

USERVER_NAMESPACE_END
