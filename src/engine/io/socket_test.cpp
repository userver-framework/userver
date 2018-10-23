#include <gtest/gtest.h>

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
    uint16_t port = 0;
    io::Addr listen_addr;
    auto listen_sock = [&] {
      io::AddrStorage addr_storage;
      auto* sa = addr_storage.As<struct sockaddr_in6>();
      sa->sin6_family = AF_INET6;
      sa->sin6_addr = in6addr_loopback;

      int attempts = 100;
      while (attempts--) {
        sa->sin6_port = port = 1024 + (rand() % (65536 - 1024));
        listen_addr = io::Addr(addr_storage, SOCK_STREAM, 0);

        try {
          return io::Listen(listen_addr);
        } catch (const std::system_error&) {
          // retry
        }
      }
      throw std::runtime_error("Could not find a port to listen");
    }();

    EXPECT_EQ(port, GetPort(listen_sock.Getsockname()));
    EXPECT_EQ("::1", listen_sock.Getsockname().RemoteAddress());

    const int old_reuseaddr = listen_sock.GetOption(SOL_SOCKET, SO_REUSEADDR);
    listen_sock.SetOption(SOL_SOCKET, SO_REUSEADDR, !old_reuseaddr);
    EXPECT_EQ(!old_reuseaddr, listen_sock.GetOption(SOL_SOCKET, SO_REUSEADDR));
    listen_sock.SetOption(SOL_SOCKET, SO_REUSEADDR, old_reuseaddr);
    EXPECT_EQ(old_reuseaddr, listen_sock.GetOption(SOL_SOCKET, SO_REUSEADDR));

    EXPECT_THROW(listen_sock.Accept(
                     Deadline::FromDuration(std::chrono::milliseconds(10))),
                 io::ConnectTimeout);

    engine::Mutex ports_mutex;
    engine::ConditionVariable ports_cv;
    bool are_ports_filled = false;

    uint16_t first_client_port = 0;
    uint16_t second_client_port = 0;
    auto listener = engine::Async([&] {
      auto first_client = listen_sock.Accept({});
      EXPECT_TRUE(first_client.IsOpen());
      auto second_client = listen_sock.Accept({});
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
      EXPECT_EQ(port, GetPort(first_client.Getsockname()));
      EXPECT_EQ(port, GetPort(second_client.Getsockname()));

      char c = 0;
      second_client.RecvSome(&c, 1, {});
      EXPECT_EQ('2', c);
      first_client.RecvAll(&c, 1, {});
      EXPECT_EQ('1', c);
    });

    auto first_client = io::Connect(listen_addr, {});
    EXPECT_TRUE(first_client.IsOpen());
    auto second_client = io::Connect(listen_addr, {});
    EXPECT_TRUE(second_client.IsOpen());

    {
      std::unique_lock<engine::Mutex> lock(ports_mutex);
      ports_cv.Wait(lock, [&] { return are_ports_filled; });
    }

    EXPECT_EQ(first_client_port, GetPort(first_client.Getsockname()));
    EXPECT_EQ(port, GetPort(first_client.Getpeername()));
    EXPECT_EQ(second_client_port, GetPort(second_client.Getsockname()));
    EXPECT_EQ(port, GetPort(second_client.Getpeername()));

    first_client.SendAll("1", 1, {});
    second_client.SendAll("2", 1, {});
    listener.Get();
  });
}
