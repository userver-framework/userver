#include <userver/net/blocking/connect_tcp_by_name.hpp>

#include <userver/utest/utest.hpp>

#include <chrono>

#include <engine/io/tests/net_listener.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

// @snippet src/net/blocking/connect_tcp_by_name_test.cpp ConnectTcpByName blocking localhost
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

USERVER_NAMESPACE_END
