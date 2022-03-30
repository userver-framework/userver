#include <userver/utest/utest.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <string_view>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/net_listener.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = utest::TcpListener;
using UdpListener = utest::UdpListener;

}  // namespace

UTEST(Socket, ConnectFail) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  io::Sockaddr addr;
  auto* sa = addr.As<sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;
  sa->sin6_port =
      htons(23);  // if you have telnet running, I have a few questions...

  io::Socket telnet_socket{addr.Domain(), io::SocketType::kStream};
  try {
    telnet_socket.Connect(addr, test_deadline);
    FAIL() << "Connection to 23/tcp succeeded";
  } catch (const io::IoSystemError& ex) {
    // oh come on, system and generic categories don't match =/
    // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
    EXPECT_EQ(static_cast<int>(std::errc::connection_refused),
              ex.Code().value());
  }
}

UTEST(Socket, ListenConnect) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener listener;

  EXPECT_EQ(listener.port, listener.socket.Getsockname().Port());
  EXPECT_EQ("::1", listener.socket.Getsockname().PrimaryAddressString());

  const int old_reuseaddr = listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR);
  listener.socket.SetOption(SOL_SOCKET, SO_REUSEADDR, !old_reuseaddr);
  EXPECT_EQ(!old_reuseaddr,
            listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR));
  listener.socket.SetOption(SOL_SOCKET, SO_REUSEADDR, old_reuseaddr);
  EXPECT_EQ(old_reuseaddr, listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR));

  UEXPECT_THROW([[maybe_unused]] auto socket = listener.socket.Accept(
                    Deadline::FromDuration(std::chrono::milliseconds(10))),
                io::IoTimeout);

  engine::Mutex ports_mutex;
  engine::ConditionVariable ports_cv;
  bool are_ports_filled = false;

  uint16_t first_client_port = 0;
  uint16_t second_client_port = 0;
  auto listen_task = engine::AsyncNoSpan([&] {
    auto first_client = listener.socket.Accept(test_deadline);
    EXPECT_TRUE(first_client.IsValid());
    auto second_client = listener.socket.Accept(test_deadline);
    EXPECT_TRUE(second_client.IsValid());

    EXPECT_EQ("::1", first_client.Getsockname().PrimaryAddressString());
    EXPECT_EQ("::1", first_client.Getpeername().PrimaryAddressString());
    EXPECT_EQ("::1", second_client.Getsockname().PrimaryAddressString());
    EXPECT_EQ("::1", second_client.Getpeername().PrimaryAddressString());

    {
      std::lock_guard<engine::Mutex> lock(ports_mutex);
      first_client_port = first_client.Getpeername().Port();
      second_client_port = second_client.Getpeername().Port();
      are_ports_filled = true;
      ports_cv.NotifyOne();
    }
    EXPECT_EQ(listener.port, first_client.Getsockname().Port());
    EXPECT_EQ(listener.port, second_client.Getsockname().Port());

    char c = 0;
    ASSERT_EQ(1, second_client.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('2', c);
    ASSERT_EQ(1, first_client.RecvAll(&c, 1, test_deadline));
    EXPECT_EQ('1', c);
  });

  io::Socket first_client{listener.addr.Domain(), listener.type};
  EXPECT_TRUE(first_client.IsValid());
  first_client.Connect(listener.addr, test_deadline);
  io::Socket second_client{listener.addr.Domain(), listener.type};
  EXPECT_TRUE(second_client.IsValid());
  second_client.Connect(listener.addr, test_deadline);

  {
    std::unique_lock<engine::Mutex> lock(ports_mutex);
    ASSERT_TRUE(ports_cv.Wait(lock, [&] { return are_ports_filled; }));
  }

  EXPECT_EQ(first_client_port, first_client.Getsockname().Port());
  EXPECT_EQ(listener.port, first_client.Getpeername().Port());
  EXPECT_EQ(second_client_port, second_client.Getsockname().Port());
  EXPECT_EQ(listener.port, second_client.Getpeername().Port());

  ASSERT_EQ(1, first_client.SendAll("1", 1, test_deadline));
  ASSERT_EQ(1, second_client.SendAll("2", 1, test_deadline));
  listen_task.Get();
}

UTEST(Socket, ReleaseReuse) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener listener;

  io::Socket client{listener.addr.Domain(), listener.type};
  client.Connect(listener.addr, test_deadline);
  const int old_fd = client.Fd();

  int fd = -1;
  while (fd != old_fd) {
    EXPECT_EQ(0, ::close(std::move(client).Release()));
    client = engine::io::Socket{listener.addr.Domain(), listener.type};
    UASSERT_NO_THROW(client.Connect(listener.addr, test_deadline));
    fd = client.Fd();
  }
}

UTEST(Socket, Closed) {
  io::Socket closed_socket;
  EXPECT_FALSE(closed_socket.IsValid());
  EXPECT_EQ(io::kInvalidFd, closed_socket.Fd());
  EXPECT_EQ(io::kInvalidFd, std::move(closed_socket).Release());
}

UTEST(Socket, Cancel) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener listener;
  auto socket_pair = listener.MakeSocketPair(test_deadline);

  engine::SingleConsumerEvent has_started_event;
  auto check_is_cancelling = [&](const char* io_op_text, auto io_op) {
    auto io_task = engine::AsyncNoSpan([&] {
      has_started_event.Send();
      io_op();
    });
    if (!has_started_event.WaitForEvent()) {
      return ::testing::AssertionFailure() << "io_task did not start";
    }
    io_task.RequestCancel();
    try {
      io_task.Get();
    } catch (const io::IoCancelled&) {
      return ::testing::AssertionSuccess();
    } catch (const std::exception&) {
      return ::testing::AssertionFailure()
             << "io operaton " << io_op_text
             << " did throw something other than IoCancelled";
    }
    return ::testing::AssertionFailure()
           << "io operation " << io_op_text << " did not throw IoCancelled";
  };

  std::vector<char> buf(socket_pair.first.GetOption(SOL_SOCKET, SO_SNDBUF) *
                        16);
  EXPECT_PRED_FORMAT1(check_is_cancelling, [&] {
    [[maybe_unused]] auto received =
        socket_pair.first.RecvSome(buf.data(), 1, test_deadline);
  });
  EXPECT_PRED_FORMAT1(check_is_cancelling, [&] {
    [[maybe_unused]] auto received =
        socket_pair.first.RecvAll(buf.data(), 1, test_deadline);
  });
  EXPECT_PRED_FORMAT1(check_is_cancelling, [&] {
    [[maybe_unused]] auto sent =
        socket_pair.first.SendAll(buf.data(), buf.size(), test_deadline);
  });
  EXPECT_PRED_FORMAT1(check_is_cancelling, [&] {
    [[maybe_unused]] auto socket = listener.socket.Accept(test_deadline);
  });
}

UTEST(Socket, ErrorPeername) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  TcpListener listener;
  engine::io::Socket client{listener.addr.Domain(), listener.type};
  client.Connect(listener.addr, test_deadline);
  listener.socket.Accept(test_deadline).Close();

  try {
    while (!test_deadline.IsReached()) {
      EXPECT_EQ(1, client.SendAll("1", 1, test_deadline));
    }
    FAIL() << "no exception on write to a closed socket";
  } catch (const io::IoTimeout&) {
    FAIL() << "no exception on write to a closed socket";
  } catch (const io::IoSystemError& ex) {
    std::errc error_value{ex.Code().value()};
    EXPECT_TRUE(
        // MAC_COMPAT: can occur due to race with socket teardown in kernel.
        // We're only interested in message so no need to check further.
        error_value == std::errc::wrong_protocol_type ||
        error_value == std::errc::broken_pipe ||
        error_value == std::errc::connection_reset ||
        error_value == std::errc::connection_aborted);
    EXPECT_NE(std::string_view{ex.what()}.find(fmt::to_string(listener.addr)),
              std::string_view::npos);
  }
}

UTEST(Socket, DomainMismatch) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  UdpListener listener;

  engine::io::Socket unix_socket{engine::io::AddrDomain::kUnix, listener.type};

  UEXPECT_THROW(unix_socket.Connect(listener.addr, test_deadline),
                io::AddrException);
  UEXPECT_THROW([[maybe_unused]] auto ret =
                    unix_socket.SendAllTo(listener.addr, "1", 1, test_deadline),
                io::AddrException);
}

UTEST(Socket, DgramBound) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  UdpListener listener;
  EXPECT_EQ("::1", listener.socket.Getsockname().PrimaryAddressString());
  EXPECT_EQ(listener.port, listener.socket.Getsockname().Port());

  std::atomic<uint16_t> client_port{0};
  auto listen_task = engine::AsyncNoSpan([&] {
    auto& server = listener.socket;
    char c = 0;
    auto server_recvfrom = server.RecvSomeFrom(&c, 1, test_deadline);
    EXPECT_EQ(1, server_recvfrom.bytes_received);
    EXPECT_EQ('1', c);
    EXPECT_EQ("::1", server_recvfrom.src_addr.PrimaryAddressString());
    EXPECT_EQ(client_port, server_recvfrom.src_addr.Port());
    UEXPECT_THROW(
        [[maybe_unused]] auto ret = server.SendAll("2", 1, test_deadline),
        io::IoSystemError);
    EXPECT_EQ(
        1, server.SendAllTo(server_recvfrom.src_addr, "2", 1, test_deadline));
    EXPECT_EQ(1, server.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('3', c);
    EXPECT_EQ(
        1, server.SendAllTo(server_recvfrom.src_addr, "4", 1, test_deadline));
  });

  engine::io::Socket client{listener.addr.Domain(), listener.type};
  client.Connect(listener.addr, test_deadline);
  client_port = client.Getsockname().Port();
  EXPECT_EQ(1, client.SendAll("1", 1, test_deadline));
  char c = 0;
  EXPECT_EQ(1, client.RecvSome(&c, 1, test_deadline));
  EXPECT_EQ('2', c);
  EXPECT_EQ(1, client.SendAll("3", 1, test_deadline));
  auto client_recvfrom = client.RecvSomeFrom(&c, 1, test_deadline);
  EXPECT_EQ(1, client_recvfrom.bytes_received);
  EXPECT_EQ('4', c);
  EXPECT_EQ(fmt::to_string(listener.addr),
            fmt::to_string(client_recvfrom.src_addr));
  listen_task.Get();
}

UTEST(Socket, DgramUnbound) {
  const auto test_deadline = Deadline::FromDuration(utest::kMaxTestWaitTime);

  UdpListener listener;
  EXPECT_EQ("::1", listener.socket.Getsockname().PrimaryAddressString());
  EXPECT_EQ(listener.port, listener.socket.Getsockname().Port());

  auto listen_task = engine::AsyncNoSpan([&] {
    auto& server = listener.socket;
    char c = 0;
    auto server_recvfrom = server.RecvSomeFrom(&c, 1, test_deadline);
    EXPECT_EQ(1, server_recvfrom.bytes_received);
    EXPECT_EQ('1', c);
    EXPECT_EQ("::1", server_recvfrom.src_addr.PrimaryAddressString());
    UEXPECT_THROW(
        [[maybe_unused]] auto ret = server.SendAll("2", 1, test_deadline),
        io::IoSystemError);
    EXPECT_EQ(
        1, server.SendAllTo(server_recvfrom.src_addr, "2", 1, test_deadline));
    EXPECT_EQ(1, server.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('3', c);
    EXPECT_EQ(
        1, server.SendAllTo(server_recvfrom.src_addr, "4", 1, test_deadline));
  });

  engine::io::Socket client{listener.addr.Domain(), listener.type};
  UEXPECT_THROW(
      [[maybe_unused]] auto ret = client.SendAll("1", 1, test_deadline),
      io::IoSystemError);
  EXPECT_EQ(1, client.SendAllTo(listener.addr, "1", 1, test_deadline));
  char c = 0;
  EXPECT_EQ(1, client.RecvSome(&c, 1, test_deadline));
  EXPECT_EQ('2', c);
  EXPECT_EQ(1, client.SendAllTo(listener.addr, "3", 1, test_deadline));
  auto client_recvfrom = client.RecvSomeFrom(&c, 1, test_deadline);
  EXPECT_EQ(1, client_recvfrom.bytes_received);
  EXPECT_EQ('4', c);
  EXPECT_EQ(fmt::to_string(listener.addr),
            fmt::to_string(client_recvfrom.src_addr));
  listen_task.Get();
}

USERVER_NAMESPACE_END
