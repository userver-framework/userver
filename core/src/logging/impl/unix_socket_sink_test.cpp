#include "unix_socket_sink.hpp"

#include <sys/uio.h>

#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/utest.hpp>

#include "sink_helper_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

engine::io::Socket MakeUnixSocketListener(const std::string& path) {
    engine::io::Socket socket{engine::io::AddrDomain::kUnix, engine::io::SocketType::kStream};
    fs::blocking::RemoveSingleFile(path);
    socket.Bind(engine::io::Sockaddr::MakeUnixSocketAddress(path));
    socket.Listen();
    return socket;
};

}  // namespace

UTEST(UnixSocketSink, SocketConnectError) {
    const auto socket_file = fs::blocking::TempFile::Create();
    EXPECT_THROW(logging::impl::UnixSocketSink{socket_file.GetPath()}, engine::io::IoSystemError);
}

UTEST(UnixSocketSink, SinkReadOnce) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    const auto socket_file = fs::blocking::TempFile::Create();
    auto listener = MakeUnixSocketListener(socket_file.GetPath());

    auto listen_task = engine::AsyncNoTracing([&listener, &deadline] {
        auto sock = listener.Accept(deadline);
        const auto data = test::ReadFromSocket(std::move(sock));
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], "message");
    });

    logging::impl::UnixSocketSink sink{socket_file.GetPath()};
    EXPECT_NO_THROW(sink.Log("message\n"));
    sink.Close();
    listen_task.Get();
}

UTEST(UnixSocketSink, SinkReadMore) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    const auto socket_file = fs::blocking::TempFile::Create();
    auto listener = MakeUnixSocketListener(socket_file.GetPath());

    auto listen_task = engine::AsyncNoTracing([&listener, &deadline] {
        auto sock = listener.Accept(deadline);
        const auto logs = test::ReadFromSocket(std::move(sock));
        ASSERT_EQ(logs.size(), 3);
        EXPECT_EQ(logs[0], "message");
        EXPECT_EQ(logs[1], "message 2");
        EXPECT_EQ(logs[2], "message 3");
    });

    logging::impl::UnixSocketSink sink{socket_file.GetPath()};
    EXPECT_NO_THROW(sink.Log("message\n"));
    EXPECT_NO_THROW(sink.Log("message 2\n"));
    EXPECT_NO_THROW(sink.Log("message 3\n"));
    sink.Close();
    listen_task.Get();
}

UTEST(UnixSocketSink, WriteIoVec) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    const auto socket_file = fs::blocking::TempFile::Create();
    auto listener = MakeUnixSocketListener(socket_file.GetPath());

    auto listen_task = engine::AsyncNoTracing([&listener, &deadline] {
        auto sock = listener.Accept(deadline);
        const auto logs = test::ReadFromSocket(std::move(sock));
        ASSERT_EQ(logs.size(), 3);
        EXPECT_EQ(logs[0], "message");
        EXPECT_EQ(logs[1], "message 2");
        EXPECT_EQ(logs[2], "message 3");
    });

    logging::impl::UnixSocketSink sink{socket_file.GetPath()};

    const std::string_view part1 = "message\n";
    const std::string_view part2 = "message 2\n";
    const std::string_view part3 = "message 3\n";
    struct iovec iov[] = {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        {.iov_base = const_cast<char*>(part1.data()), .iov_len = part1.size()},
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        {.iov_base = const_cast<char*>(part2.data()), .iov_len = part2.size()},
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        {.iov_base = const_cast<char*>(part3.data()), .iov_len = part3.size()},
    };
    EXPECT_NO_THROW(sink.Write(std::span<const struct iovec>{iov}));
    sink.Close();
    listen_task.Get();
}

USERVER_NAMESPACE_END
