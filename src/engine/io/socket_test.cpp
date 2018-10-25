#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>

#include <engine/async.hpp>
#include <engine/condition_variable.hpp>
#include <engine/io/addr.hpp>
#include <engine/io/socket.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>
#include <utest/utest.hpp>

namespace {

namespace io = engine::io;
using Deadline = engine::Deadline;

uint16_t GetPort(const io::Addr& addr) {
  auto* sa = addr.As<struct sockaddr_in6>();
  return sa ? sa->sin6_port : 0;
}

struct Listener {
  Listener() : port(0) {
    io::AddrStorage addr_storage;
    auto* sa = addr_storage.As<struct sockaddr_in6>();
    sa->sin6_family = AF_INET6;
    sa->sin6_addr = in6addr_loopback;

    int attempts = 100;
    while (attempts--) {
      sa->sin6_port = port = htons(1024 + (rand() % (65536 - 1024)));
      addr = io::Addr(addr_storage, SOCK_STREAM, 0);

      try {
        socket = io::Listen(addr);
        return;
      } catch (const std::system_error& ex) {
        // retry
      }
    }
    throw std::runtime_error("Could not find a port to listen");
  };

  uint16_t port;
  io::Addr addr;
  io::Socket socket;
};

}  // namespace

TEST(Socket, ConnectFail) {
  RunInCoro([] {
    io::AddrStorage addr_storage;
    auto* sa = addr_storage.As<sockaddr_in6>();
    sa->sin6_family = AF_INET6;
    sa->sin6_addr = in6addr_loopback;
    sa->sin6_port = 23;  // if you have telnet running, I have a few quesions...

    try {
      io::Connect(io::Addr(addr_storage, SOCK_STREAM, 0), {});
      FAIL() << "Connection to 23/tcp succeeded";
    } catch (const std::system_error& ex) {
      // oh come on, system and generic categories don't match =/
      EXPECT_EQ(static_cast<int>(std::errc::connection_refused),
                ex.code().value());
    }
  });
}

TEST(Socket, ListenConnect) {
  RunInCoro([] {
    Listener listener;

    EXPECT_EQ(listener.port, GetPort(listener.socket.Getsockname()));
    EXPECT_EQ("::1", listener.socket.Getsockname().RemoteAddress());

    const int old_reuseaddr =
        listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR);
    listener.socket.SetOption(SOL_SOCKET, SO_REUSEADDR, !old_reuseaddr);
    EXPECT_EQ(!old_reuseaddr,
              listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR));
    listener.socket.SetOption(SOL_SOCKET, SO_REUSEADDR, old_reuseaddr);
    EXPECT_EQ(old_reuseaddr,
              listener.socket.GetOption(SOL_SOCKET, SO_REUSEADDR));

    EXPECT_THROW(listener.socket.Accept(
                     Deadline::FromDuration(std::chrono::milliseconds(10))),
                 io::ConnectTimeout);

    engine::Mutex ports_mutex;
    engine::ConditionVariable ports_cv;
    bool are_ports_filled = false;

    uint16_t first_client_port = 0;
    uint16_t second_client_port = 0;
    auto listen_task = engine::Async([&] {
      auto first_client = listener.socket.Accept({});
      EXPECT_TRUE(first_client.IsOpen());
      auto second_client = listener.socket.Accept({});
      EXPECT_TRUE(second_client.IsOpen());

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
      second_client.RecvSome(&c, 1, {});
      EXPECT_EQ('2', c);
      first_client.RecvAll(&c, 1, {});
      EXPECT_EQ('1', c);
    });

    auto first_client = io::Connect(listener.addr, {});
    EXPECT_TRUE(first_client.IsOpen());
    auto second_client = io::Connect(listener.addr, {});
    EXPECT_TRUE(second_client.IsOpen());

    {
      std::unique_lock<engine::Mutex> lock(ports_mutex);
      ports_cv.Wait(lock, [&] { return are_ports_filled; });
    }

    EXPECT_EQ(first_client_port, GetPort(first_client.Getsockname()));
    EXPECT_EQ(listener.port, GetPort(first_client.Getpeername()));
    EXPECT_EQ(second_client_port, GetPort(second_client.Getsockname()));
    EXPECT_EQ(listener.port, GetPort(second_client.Getpeername()));

    first_client.SendAll("1", 1, {});
    second_client.SendAll("2", 1, {});
    listen_task.Get();
  });
}

TEST(Socket, ReleaseReuse) {
  RunInCoro([] {
    Listener listener;

    auto client = io::Connect(listener.addr, {});
    const int old_fd = client.Fd();

    int fd = -1;
    while (fd != old_fd) {
      EXPECT_EQ(0, ::close(std::move(client).Release()));
      ASSERT_NO_THROW(client = io::Connect(listener.addr, {}));
      fd = client.Fd();
    }
  });
}

TEST(Socket, Closed) {
  RunInCoro([] {
    io::Socket closed_socket;
    EXPECT_FALSE(closed_socket.IsOpen());
    EXPECT_EQ(io::Socket::kInvalidFd, closed_socket.Fd());
    EXPECT_EQ(io::Socket::kInvalidFd, std::move(closed_socket).Release());
  });
}
