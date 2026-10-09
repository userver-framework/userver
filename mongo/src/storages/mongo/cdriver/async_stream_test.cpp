#include <storages/mongo/cdriver/async_stream.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/tcp_connect_precheck.hpp>

#include <poll.h>

#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace cdriver = storages::mongo::impl::cdriver;

class StreamPeer {
public:
    explicit StreamPeer(bool experimental = true)
        : resolver_(engine::current_task::GetTaskProcessor(), {})
    {
        engine::io::Socket listener{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
        listener.Bind(engine::io::Sockaddr::MakeIPv4LoopbackAddress());
        listener.Listen();
        const cdriver::UriPtr uri{
            mongoc_uri_new(fmt::format("mongodb://127.0.0.1:{}", listener.Getsockname().Port()).c_str())
        };
        bson_error_t error{};
        stream_.reset(
            experimental
                ? cdriver::MakeAsyncStreamForNativePool(
                      uri.get(),
                      mongoc_uri_get_hosts(uri.get()),
                      &resolver_,
                      ssl_opt_,
                      &error
                  )
                : cdriver::MakeAsyncStream(uri.get(), mongoc_uri_get_hosts(uri.get()), &legacy_init_data_, &error)
        );
        UINVARIANT(stream_, error.message);
        peer_ = listener.Accept(engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
    }

    void Send(std::string_view data) {
        ASSERT_EQ(
            peer_.SendAll(data.data(), data.size(), engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
            data.size()
        );
    }

    mongoc_stream_t* GetStream() const { return stream_.get(); }

    char ReadOne() const {
        char byte{};
        mongoc_iovec_t iov{&byte, 1};
        EXPECT_EQ(mongoc_stream_readv(stream_.get(), &iov, 1, 1, 1000), 1);
        return byte;
    }

private:
    clients::dns::Resolver resolver_;
    mongoc_ssl_opt_t ssl_opt_{};

    cdriver::AsyncStreamInitiatorData legacy_init_data_{&resolver_, {}, {}};

    engine::io::Socket peer_;
    cdriver::StreamPtr stream_;
};

}  // namespace

UTEST(MongocStream, BothTransportsReadAndPoll) {
    for (const bool experimental : {false, true}) {
        StreamPeer connection{experimental};
        connection.Send("abc");
        mongoc_stream_poll_t poll{connection.GetStream(), POLLIN, 0};
        EXPECT_EQ(mongoc_stream_poll(&poll, 1, 1000), 1);
        EXPECT_TRUE(poll.revents & POLLIN);
        EXPECT_EQ(connection.ReadOne(), 'a');
        EXPECT_EQ(connection.ReadOne(), 'b');
        EXPECT_EQ(connection.ReadOne(), 'c');
    }
}

UTEST(MongocStream, IgnoresLegacyHostBlock) {
    namespace impl = storages::mongo::impl;
    constexpr int kErrorsToBlockHost = 5;
    engine::io::Socket listener{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    listener.Bind(engine::io::Sockaddr::MakeIPv4LoopbackAddress());
    listener.Listen();
    const cdriver::UriPtr uri{
        mongoc_uri_new(fmt::format("mongodb://127.0.0.1:{}", listener.Getsockname().Port()).c_str())
    };
    const auto* host = mongoc_uri_get_hosts(uri.get());
    impl::ReportTcpConnectSuccess(host->host_and_port);
    for (int i = 0; i < kErrorsToBlockHost; ++i) {
        impl::ReportTcpConnectError(host->host_and_port);
    }
    clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(), {}};
    mongoc_ssl_opt_t ssl_opt{};
    bson_error_t error{};
    const cdriver::StreamPtr stream{cdriver::MakeAsyncStreamForNativePool(uri.get(), host, &resolver, ssl_opt, &error)};
    EXPECT_TRUE(stream) << error.message;
    EXPECT_NE(impl::CheckTcpConnectionState(host->host_and_port), impl::HostConnectionState::kAlive);
    impl::ReportTcpConnectSuccess(host->host_and_port);
}

UTEST(MongocStream, ConnectionErrorsDoNotBlockLegacyHost) {
    namespace impl = storages::mongo::impl;
    constexpr int kConnectionAttempts = 6;
    engine::io::Socket reserved{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    reserved.Bind(engine::io::Sockaddr::MakeIPv4LoopbackAddress());
    const cdriver::UriPtr uri{
        mongoc_uri_new(fmt::format("mongodb://127.0.0.1:{}", reserved.Getsockname().Port()).c_str())
    };
    const auto* host = mongoc_uri_get_hosts(uri.get());
    impl::ReportTcpConnectSuccess(host->host_and_port);
    clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(), {}};
    mongoc_ssl_opt_t ssl_opt{};
    for (int i = 0; i < kConnectionAttempts; ++i) {
        bson_error_t error{};
        const cdriver::StreamPtr stream{
            cdriver::MakeAsyncStreamForNativePool(uri.get(), host, &resolver, ssl_opt, &error)
        };
        EXPECT_FALSE(stream);
        EXPECT_EQ(error.code, MONGOC_ERROR_STREAM_CONNECT);
    }
    EXPECT_EQ(impl::CheckTcpConnectionState(host->host_and_port), impl::HostConnectionState::kAlive);
    impl::ReportTcpConnectSuccess(host->host_and_port);
}

UTEST(MongocStream, PollBufferedData) {
    StreamPeer connection;
    connection.Send("ab");
    EXPECT_EQ(connection.ReadOne(), 'a');
    mongoc_stream_poll_t poll{connection.GetStream(), POLLIN, 0};
    EXPECT_EQ(mongoc_stream_poll(&poll, 1, 10), 1);
    EXPECT_TRUE(poll.revents & POLLIN);
    EXPECT_EQ(connection.ReadOne(), 'b');
}

UTEST(MongocStream, RepeatedPollAfterTimeoutAndRead) {
    StreamPeer connection;
    mongoc_stream_poll_t poll{connection.GetStream(), POLLIN, 0};
    EXPECT_EQ(mongoc_stream_poll(&poll, 1, 1), 0);
    for (char value : std::string_view{"abc"}) {
        connection.Send(std::string_view{&value, 1});
        EXPECT_EQ(mongoc_stream_poll(&poll, 1, 1000), 1);
        EXPECT_TRUE(poll.revents & POLLIN);
        EXPECT_EQ(connection.ReadOne(), value);
        EXPECT_EQ(mongoc_stream_poll(&poll, 1, 1), 0);
    }
}

UTEST(MongocStream, PollChangingStreamSet) {
    StreamPeer first;
    StreamPeer second;
    mongoc_stream_poll_t first_poll{first.GetStream(), POLLIN, 0};
    EXPECT_EQ(mongoc_stream_poll(&first_poll, 1, 1), 0);
    first.Send("x");
    second.Send("y");
    mongoc_stream_poll_t second_poll{second.GetStream(), POLLIN, 0};
    EXPECT_EQ(mongoc_stream_poll(&second_poll, 1, 1000), 1);
    EXPECT_TRUE(second_poll.revents & POLLIN);
    EXPECT_EQ(second.ReadOne(), 'y');
    EXPECT_EQ(mongoc_stream_poll(&first_poll, 1, 1000), 1);
    EXPECT_TRUE(first_poll.revents & POLLIN);
    EXPECT_EQ(first.ReadOne(), 'x');
}

USERVER_NAMESPACE_END
