#include <userver/net/connect_tcp_by_name.hpp>

#include <userver/utest/utest.hpp>

#include <chrono>
#include <cstdint>

#include <engine/io/tests/net_listener.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/dns_server_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct LoopbackResolverFixture : public ::testing::Test {
    LoopbackResolverFixture()
        : hosts_file{[] {
              auto file = fs::blocking::TempFile::Create();
              fs::blocking::RewriteFileContents(file.GetPath(), "127.0.0.1 localhost\n::1 localhost\n");
              return file;
          }()},
          dns_mock{[](const utest::DnsServerMock::DnsQuery& query) {
              utest::DnsServerMock::DnsAnswerVector result;
              if (query.type == utest::DnsServerMock::RecordType::kA) {
                  auto addr = engine::io::Sockaddr::MakeIPv4LoopbackAddress();
                  result.push_back({query.type, addr, 60});
              } else if (query.type == utest::DnsServerMock::RecordType::kAAAA) {
                  auto addr = engine::io::Sockaddr::MakeLoopbackAddress();
                  result.push_back({query.type, addr, 60});
              }
              return result;
          }},
          resolver{
              engine::current_task::GetTaskProcessor(),
              [this] {
                  ::userver::static_config::DnsClient config;
                  config.hosts_file_path = hosts_file.GetPath();
                  config.hosts_file_update_interval = utest::kMaxTestWaitTime;
                  config.network_timeout = utest::kMaxTestWaitTime;
                  config.network_attempts = 1;
                  config.cache_max_reply_ttl = std::chrono::seconds{60};
                  config.cache_failure_ttl = std::chrono::seconds{60};
                  config.cache_ways = 1;
                  config.cache_size_per_way = 4;
                  config.network_custom_servers = {dns_mock.GetServerAddress()};
                  return config;
              }()
          }
    {}

    fs::blocking::TempFile hosts_file;
    utest::DnsServerMock dns_mock;
    clients::dns::Resolver resolver;
};

}  // namespace

// @snippet src/net/connect_tcp_by_name_test.cpp ConnectTcpByName localhost
UTEST_F(LoopbackResolverFixture, ConnectTcpByNameLocalhost) {
    engine::io::tests::TcpListener listener;
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    /// [ConnectTcpByName localhost]
    auto socket = net::ConnectTcpByName("localhost", listener.Port(), resolver, deadline);
    /// [ConnectTcpByName localhost]

    ASSERT_TRUE(socket.IsValid());
    EXPECT_EQ(listener.addr.PrimaryAddressString(), socket.Getpeername().PrimaryAddressString());
    EXPECT_EQ(listener.Port(), socket.Getpeername().Port());
}

UTEST_F(LoopbackResolverFixture, ConnectTcpByNameSocketCommunicates) {
    engine::io::tests::TcpListener listener;
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    auto client_socket = net::ConnectTcpByName(
        "localhost",
        listener.Port(),
        resolver,
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
