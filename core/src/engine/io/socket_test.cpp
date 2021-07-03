#include <utest/utest.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <engine/async.hpp>
#include <engine/condition_variable.hpp>
#include <engine/io/addr.hpp>
#include <engine/io/socket.hpp>
#include <engine/io/util_test.hpp>
#include <engine/mutex.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;
using TcpListener = io::util_test::TcpListener;

}  // namespace

UTEST(Socket, ConnectFail) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  io::AddrStorage addr_storage;
  auto* sa = addr_storage.As<sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;
  sa->sin6_port = 23;  // if you have telnet running, I have a few quesions...

  try {
    [[maybe_unused]] auto telnet =
        io::Connect(io::Addr(addr_storage, SOCK_STREAM, 0), test_deadline);
    FAIL() << "Connection to 23/tcp succeeded";
  } catch (const io::IoSystemError& ex) {
    // oh come on, system and generic categories don't match =/
    // default_error_category() fixed only in GCC 9.1 (PR libstdc++/60555)
    EXPECT_EQ(static_cast<int>(std::errc::connection_refused),
              ex.Code().value());
  }
}

UTEST(Socket, ListenConnect) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener listener;

  EXPECT_EQ(listener.port, GetPort(listener.socket.Getsockname()));
  EXPECT_EQ("::1", listener.socket.Getsockname().RemoteAddress());

  const int old_reuseaddr = listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR);
  listener.socket.SetOption(SOL_SOCKET, SO_REUSEADDR, !old_reuseaddr);
  EXPECT_EQ(!old_reuseaddr,
            listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR));
  listener.socket.SetOption(SOL_SOCKET, SO_REUSEADDR, old_reuseaddr);
  EXPECT_EQ(old_reuseaddr, listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR));

  EXPECT_THROW([[maybe_unused]] auto socket = listener.socket.Accept(
                   Deadline::FromDuration(std::chrono::milliseconds(10))),
               io::ConnectTimeout);

  engine::Mutex ports_mutex;
  engine::ConditionVariable ports_cv;
  bool are_ports_filled = false;

  uint16_t first_client_port = 0;
  uint16_t second_client_port = 0;
  auto listen_task = engine::impl::Async([&] {
    auto first_client = listener.socket.Accept(test_deadline);
    EXPECT_TRUE(first_client.IsValid());
    auto second_client = listener.socket.Accept(test_deadline);
    EXPECT_TRUE(second_client.IsValid());

    EXPECT_EQ("::1", first_client.Getsockname().RemoteAddress());
    EXPECT_EQ("::1", first_client.Getpeername().RemoteAddress());
    EXPECT_EQ("::1", second_client.Getsockname().RemoteAddress());
    EXPECT_EQ("::1", second_client.Getpeername().RemoteAddress());

    {
      std::lock_guard<engine::Mutex> lock(ports_mutex);
      first_client_port = GetPort(first_client.Getpeername());
      second_client_port = GetPort(second_client.Getpeername());
      are_ports_filled = true;
      ports_cv.NotifyOne();
    }
    EXPECT_EQ(listener.port, GetPort(first_client.Getsockname()));
    EXPECT_EQ(listener.port, GetPort(second_client.Getsockname()));

    char c = 0;
    ASSERT_EQ(1, second_client.RecvSome(&c, 1, test_deadline));
    EXPECT_EQ('2', c);
    ASSERT_EQ(1, first_client.RecvAll(&c, 1, test_deadline));
    EXPECT_EQ('1', c);
  });

  auto first_client = io::Connect(listener.addr, test_deadline);
  EXPECT_TRUE(first_client.IsValid());
  auto second_client = io::Connect(listener.addr, test_deadline);
  EXPECT_TRUE(second_client.IsValid());

  {
    std::unique_lock<engine::Mutex> lock(ports_mutex);
    ASSERT_TRUE(ports_cv.Wait(lock, [&] { return are_ports_filled; }));
  }

  EXPECT_EQ(first_client_port, GetPort(first_client.Getsockname()));
  EXPECT_EQ(listener.port, GetPort(first_client.Getpeername()));
  EXPECT_EQ(second_client_port, GetPort(second_client.Getsockname()));
  EXPECT_EQ(listener.port, GetPort(second_client.Getpeername()));

  ASSERT_EQ(1, first_client.SendAll("1", 1, test_deadline));
  ASSERT_EQ(1, second_client.SendAll("2", 1, test_deadline));
  listen_task.Get();
}

UTEST(Socket, ReleaseReuse) {
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener listener;

  auto client = io::Connect(listener.addr, test_deadline);
  const int old_fd = client.Fd();

  int fd = -1;
  while (fd != old_fd) {
    EXPECT_EQ(0, ::close(std::move(client).Release()));
    ASSERT_NO_THROW(client = io::Connect(listener.addr, test_deadline));
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
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener listener;
  auto socket_pair = listener.MakeSocketPair(test_deadline);

  engine::SingleConsumerEvent has_started_event;
  auto check_is_cancelling = [&](const char* io_op_text, auto io_op) {
    auto io_task = engine::impl::Async([&] {
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
  const auto test_deadline = Deadline::FromDuration(kMaxTestWaitTime);

  TcpListener listener;
  auto client = io::Connect(listener.addr, test_deadline);
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
    EXPECT_TRUE(::strstr(ex.what(), ToString(listener.addr).c_str()));
  }
}
