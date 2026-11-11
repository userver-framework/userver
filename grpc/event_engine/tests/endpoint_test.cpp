#include <ugrpc/impl/event_engine/endpoint.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <grpc/impl/channel_arg_names.h>
#include <src/core/lib/channel/channel_args.h>
#include <src/core/lib/event_engine/channel_args_endpoint_config.h>
#include <src/core/lib/event_engine/tcp_socket_utils.h>

#include <userver/engine/async.hpp>
#include <userver/engine/multi_consumer_event.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>

#include <ugrpc/impl/event_engine/tcp_options.hpp>

#include "transport_test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine::tests {
namespace {

int GetSocketOption(int fd, int level, int name) {
    int value = 0;
    socklen_t size = sizeof(value);
    UINVARIANT(::getsockopt(fd, level, name, &value, &size) == 0, "getsockopt failed");
    return value;
}

void CheckDestructionCancelsIo(bool write) {
    Completion completion;
    EndpointFixture f;
    const int fd = f.Posix().GetWrappedFd();
    if (write) {
        constexpr int kBufferSize = 4096;
        ASSERT_EQ(::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &kBufferSize, sizeof(kBufferSize)), 0);
        const std::string payload(1024 * 1024, 'w');
        completion.buffer.Append(gee::Slice::FromCopiedString(y_absl::string_view{payload.data(), payload.size()}));
        ASSERT_FALSE(f.endpoint
                         ->Write([&](auto status) { completion.Done(std::move(status)); }, &completion.buffer, nullptr)
        );
    } else {
        ASSERT_FALSE(f.endpoint
                         ->Read([&](auto status) { completion.Done(std::move(status)); }, &completion.buffer, nullptr));
    }
    f.endpoint.reset();
    EXPECT_EQ(completion.Wait().code(), y_absl::StatusCode::kUnavailable);
    EXPECT_EQ(completion.calls, 1);
    EXPECT_EQ(completion.buffer.Length(), 0);
    f.listening.listener.reset();
    f.listening.owner.Shutdown();
    EXPECT_EQ(::fcntl(fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

UTEST(GrpcEventEngineEndpoint, EmptyAndSmallWritesCompleteInlineWithoutCallback) {
    Completion completion;
    EndpointFixture f;
    auto callback = [](auto) { ADD_FAILURE() << "Synchronous write invoked callback"; };
    EXPECT_TRUE(f.endpoint->Write(callback, &completion.buffer, nullptr));
    completion.buffer.Append(gee::Slice::FromCopiedString("hello"));
    EXPECT_TRUE(f.endpoint->Write(callback, &completion.buffer, nullptr));
    EXPECT_EQ(completion.buffer.Length(), 0);
    std::string received(5, '\0');
    EXPECT_EQ(f.peer.RecvAll(received.data(), received.size(), Deadline()), received.size());
    EXPECT_EQ(received, "hello");
}

UTEST(GrpcEventEngineEndpoint, LargeVectoredWriteResumesAfterKernelBackpressure) {
    Completion completion;
    EndpointFixture f;
    constexpr int kBufferSize = 4096;
    ASSERT_EQ(::setsockopt(f.Posix().GetWrappedFd(), SOL_SOCKET, SO_SNDBUF, &kBufferSize, sizeof(kBufferSize)), 0);
    std::string expected;
    for (int i = 0; i < 600; ++i) {
        completion.buffer.Append(gee::Slice::FromCopiedString(""));
        const std::string part(1024, static_cast<char>('a' + i % 26));
        completion.buffer.Append(gee::Slice::FromCopiedString(y_absl::string_view{part.data(), part.size()}));
        expected += part;
    }
    ASSERT_FALSE(f.endpoint
                     ->Write([&](auto status) { completion.Done(std::move(status)); }, &completion.buffer, nullptr));
    std::string received(expected.size(), '\0');
    EXPECT_EQ(f.peer.RecvAll(received.data(), received.size(), Deadline()), received.size());
    EXPECT_TRUE(completion.Wait().ok());
    EXPECT_EQ(completion.calls, 1);
    EXPECT_EQ(completion.buffer.Length(), 0);
    EXPECT_EQ(received, expected);
}

UTEST(GrpcEventEngineEndpoint, ReadHintDoesNotDelayAvailableBytesAndRetainedSlicesStayValid) {
    Completion first;
    EndpointFixture f;
    f.Send("retained");
    EXPECT_TRUE(f.Read(first, std::numeric_limits<std::int64_t>::max()).ok());
    EXPECT_EQ(Bytes(first.buffer), "retained");
    for (int i = 0; i < 8; ++i) {
        const std::string payload(256 * 1024, static_cast<char>('a' + i));
        auto writer = engine::AsyncNoTracing([&] { f.Send(payload); });
        EXPECT_EQ(f.ReadBytes(payload.size(), 64 * 1024), payload);
        writer.Get();
        EXPECT_EQ(Bytes(first.buffer), "retained");
    }
    f.endpoint.reset();
    EXPECT_EQ(Bytes(first.buffer), "retained");
}

UTEST(GrpcEventEngineEndpoint, PeerCloseCompletesPendingReadWithAnError) {
    Completion completion;
    EndpointFixture f;
    ASSERT_FALSE(f.endpoint->Read([&](auto status) { completion.Done(std::move(status)); }, &completion.buffer, nullptr)
    );
    f.peer.Close();
    EXPECT_EQ(completion.Wait().code(), y_absl::StatusCode::kUnavailable);
    EXPECT_EQ(completion.calls, 1);
}

UTEST(GrpcEventEngineEndpoint, DestructionCancelsIdleRead) { CheckDestructionCancelsIo(false); }
UTEST(GrpcEventEngineEndpoint, DestructionCancelsBlockedWrite) { CheckDestructionCancelsIo(true); }

UTEST(GrpcEventEngineEndpoint, CallbackCanDestroyTheEndpoint) {
    Completion completion;
    EndpointFixture f;
    ASSERT_FALSE(f.endpoint->Read(
        [&](auto status) {
            f.endpoint.reset();
            completion.Done(std::move(status));
        },
        &completion.buffer,
        nullptr
    ));
    f.Send("done");
    EXPECT_TRUE(completion.Wait().ok());
    EXPECT_EQ(Bytes(completion.buffer), "done");
    EXPECT_FALSE(f.endpoint);
}

UTEST(GrpcEventEngineEndpoint, NewReadAndWriteAfterShutdownFailAsynchronously) {
    Completion read;
    Completion write;
    EndpointFixture f;
    f.Posix().Shutdown({});
    ASSERT_FALSE(f.endpoint->Read([&](auto status) { read.Done(std::move(status)); }, &read.buffer, nullptr));
    ASSERT_FALSE(f.endpoint->Write([&](auto status) { write.Done(std::move(status)); }, &write.buffer, nullptr));
    EXPECT_EQ(read.calls, 0);
    EXPECT_EQ(write.calls, 0);
    EXPECT_EQ(read.Wait().code(), y_absl::StatusCode::kUnavailable);
    EXPECT_EQ(write.Wait().code(), y_absl::StatusCode::kUnavailable);
}

UTEST(GrpcEventEngineEndpoint, ReleaseKeepsTheSocketUsableAfterPendingIoIsCancelled) {
    Completion read;
    engine::Promise<y_absl::StatusOr<int>> released;
    auto release = released.get_future();
    EndpointFixture f;
    const int original = f.Posix().GetWrappedFd();
    const auto domain = static_cast<engine::io::AddrDomain>(f.endpoint->GetLocalAddress().address()->sa_family);
    ASSERT_FALSE(f.endpoint->Read([&](auto status) { read.Done(std::move(status)); }, &read.buffer, nullptr));
    f.Posix().Shutdown([&](auto fd) {
        EXPECT_EQ(read.calls, 1);
        EXPECT_TRUE(Executor::IsInCallback());
        released.set_value(std::move(fd));
    });
    f.endpoint.reset();
    EXPECT_FALSE(read.Wait().ok());
    ASSERT_EQ(release.wait_for(utest::kMaxTestWaitTime), engine::FutureStatus::kReady);
    auto result = release.get();
    ASSERT_TRUE(result.ok()) << result.status();
    engine::io::Socket owned{*result, domain};
    EXPECT_EQ(owned.Fd(), original);
    EXPECT_NE(::fcntl(original, F_GETFD), -1);
    f.Send("x");
    char byte{};
    EXPECT_EQ(owned.RecvAll(&byte, 1, Deadline()), 1);
    EXPECT_EQ(byte, 'x');
    EXPECT_EQ(owned.SendAll("y", 1, Deadline()), 1);
    EXPECT_EQ(f.peer.RecvAll(&byte, 1, Deadline()), 1);
    EXPECT_EQ(byte, 'y');
}

UTEST(GrpcEventEngineEndpoint, AcceptedEndpointInheritsListenerTcpOptions) {
    constexpr int kUserTimeoutMs = 7'000;
    constexpr int kReceiveBufferSize = 64 * 1024;
    EndpointFixture f{gee::ChannelArgsEndpointConfig{
        grpc_core::ChannelArgs{}
            .Set(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, kUserTimeoutMs)
            .Set(GRPC_ARG_TCP_RECEIVE_BUFFER_SIZE, kReceiveBufferSize)
    }};
    const int fd = f.Posix().GetWrappedFd();
    EXPECT_EQ(GetSocketOption(fd, IPPROTO_TCP, TCP_NODELAY), 1);
    EXPECT_EQ(GetSocketOption(fd, IPPROTO_TCP, TCP_USER_TIMEOUT), kUserTimeoutMs);
    EXPECT_GE(GetSocketOption(fd, SOL_SOCKET, SO_RCVBUF), kReceiveBufferSize);
}

UTEST(GrpcEventEngineEndpoint, MaxReadBufferSizeKeepsTheReadEstimateAtTheInitialChunk) {
    constexpr int kLimit = PosixTcpOptions::kDefaultReadChunkSize;
    constexpr std::size_t kBatchSize = 64 * 1024;
    constexpr std::size_t kBatches = 16;
    EndpointFixture f{gee::ChannelArgsEndpointConfig{grpc_core::ChannelArgs{}.Set(kTcpMaxReadBufferSizeArg, kLimit)}};
    const std::string payload(kBatchSize, 'r');
    for (std::size_t batch = 0; batch < kBatches; ++batch) {
        // On one worker, the next read sees EAGAIN before the queued writer starts the next batch.
        auto writer = engine::AsyncNoTracing([&] { f.Send(payload); });
        std::size_t received = 0;
        while (received < kBatchSize) {
            Completion read;
            ASSERT_TRUE(f.Read(read).ok());
            ASSERT_GT(read.buffer.Length(), 0);
            EXPECT_LE(read.buffer.Length(), static_cast<std::size_t>(kLimit));
            received += read.buffer.Length();
        }
        writer.Get();
        EXPECT_EQ(received, kBatchSize);
    }
}

void CheckConcurrentShutdown(bool write) {
    for (int iteration = 0; iteration < 32; ++iteration) {
        Completion completion;
        EndpointFixture f;
        if (write) {
            constexpr int kBufferSize = 4096;
            ASSERT_EQ(
                ::setsockopt(f.Posix().GetWrappedFd(), SOL_SOCKET, SO_SNDBUF, &kBufferSize, sizeof(kBufferSize)),
                0
            );
            const std::string data(1024 * 1024, 'w');
            completion.buffer.Append(gee::Slice::FromCopiedString(y_absl::string_view{data.data(), data.size()}));
        }
        engine::MultiConsumerEvent start;
        auto reader = engine::AsyncNoTracing([&] {
            start.Wait();
            auto callback = [&](auto status) { completion.Done(std::move(status)); };
            EXPECT_FALSE(
                write ? f.endpoint->Write(callback, &completion.buffer, nullptr)
                      : f.endpoint->Read(callback, &completion.buffer, nullptr)
            );
        });
        auto closer = engine::AsyncNoTracing([&] {
            start.Wait();
            f.Posix().Shutdown({});
        });
        start.Send();
        reader.Get();
        closer.Get();
        EXPECT_EQ(completion.Wait().code(), y_absl::StatusCode::kUnavailable);
        EXPECT_EQ(completion.calls, 1);
    }
}

UTEST_MT(GrpcEventEngineEndpoint, ShutdownRacesWithReadRegistrationExactlyOnce, 4) { CheckConcurrentShutdown(false); }
UTEST_MT(GrpcEventEngineEndpoint, ShutdownRacesWithWriteRegistrationExactlyOnce, 4) { CheckConcurrentShutdown(true); }

}  // namespace
}  // namespace ugrpc::impl::event_engine::tests

USERVER_NAMESPACE_END
