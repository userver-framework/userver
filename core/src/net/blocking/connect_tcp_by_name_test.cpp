#include <userver/net/blocking/connect_tcp_by_name.hpp>

#include <userver/utest/utest.hpp>

#include <chrono>

#include <engine/io/tests/net_listener.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>

USERVER_NAMESPACE_BEGIN

// @snippet core/src/net/blocking/connect_tcp_by_name_test.cpp ConnectTcpByName blocking localhost
UTEST(BlockingConnectTcpByName, Localhost) {
    engine::io::tests::TcpListener listener;
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    /// [ConnectTcpByName blocking localhost]
    auto socket = net::blocking::ConnectTcpByName("localhost", listener.Port(), deadline);
    /// [ConnectTcpByName blocking localhost]

    ASSERT_TRUE(socket.IsValid());
    EXPECT_EQ(listener.addr.PrimaryAddressString(), socket.Getpeername().PrimaryAddressString());
    EXPECT_EQ(listener.Port(), socket.Getpeername().Port());
}

UTEST(BlockingConnectTcpByName, SocketCommunicates) {
    engine::io::tests::TcpListener listener;
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    auto client_socket = net::blocking::ConnectTcpByName(
        "localhost",
        listener.Port(),
        engine::Deadline::FromDuration(std::chrono::seconds{5})
    );

    ASSERT_TRUE(client_socket.IsValid());

    auto [server_socket, _] = listener.MakeSocketPair(deadline);

    const char k_msg[] = "hello";
    constexpr size_t kMsgLen = sizeof(k_msg) - 1;
    ASSERT_EQ(kMsgLen, client_socket.SendAll(k_msg, kMsgLen, deadline));

    char buf[16];
    ASSERT_EQ(kMsgLen, server_socket.RecvAll(buf, kMsgLen, deadline));
    buf[kMsgLen] = '\0';
    EXPECT_STREQ(k_msg, buf);
}

UTEST(BlockingConnectTcpByName, IgnoresCancel) {
    // Prefer IPv4: on some systems getaddrinfo("localhost") returns only 127.0.0.1.
    engine::io::tests::TcpListener listener{engine::io::tests::IpVersion::kV4};
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    engine::SingleConsumerEvent started;
    auto task = engine::CriticalAsyncNoTracing([&] {
        engine::current_task::RequestCancel();
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
        started.Send();
        // Cancellations are ignored by ConnectTcpByName; interruptible versions may be added later.
        auto socket = net::blocking::ConnectTcpByName("localhost", listener.Port(), deadline);
        EXPECT_TRUE(socket.IsValid());
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
        return socket.Getpeername().Port();
    });

    ASSERT_TRUE(started.WaitForEventFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(task.Get(), listener.Port());
}

USERVER_NAMESPACE_END
