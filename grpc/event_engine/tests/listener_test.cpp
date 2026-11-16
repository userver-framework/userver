#include <ugrpc/impl/event_engine/listener.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstddef>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <grpc/impl/channel_arg_names.h>
#include <src/core/lib/channel/channel_args.h>
#include <src/core/lib/event_engine/channel_args_endpoint_config.h>
#include <src/core/lib/resource_quota/resource_quota.h>
#include <y_absl/strings/string_view.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/death_tests.hpp>

#include "transport_test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine::tests {
namespace {

bool HasIpv6Loopback() {
    auto probe = NonblockingSocket::Create(AF_INET6);
    const auto address = engine::io::Sockaddr::MakeIPSocketAddress("::1");
    return probe && probe->Bind({address.Data(), address.Size()}).ok();
}

class FailingFactory final : public gee::MemoryAllocatorFactory {
public:
    explicit FailingFactory(unsigned failures)
        : failures_(failures)
    {}

    gee::MemoryAllocator CreateMemoryAllocator(y_absl::string_view name) override {
        if (failures_ != 0) {
            --failures_;
            throw std::bad_alloc{};
        }
        return quota_->memory_quota()->CreateMemoryAllocator(name);
    }

private:
    grpc_core::ResourceQuotaRefPtr quota_{grpc_core::MakeResourceQuota("event-engine-failing-factory")};
    unsigned failures_;
};

void ConnectDroppedByServer(const engine::io::Sockaddr& address) {
    engine::io::Socket dropped{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    dropped.Connect(address, Deadline());
    char byte{};
    EXPECT_EQ(dropped.RecvSome(&byte, sizeof(byte), Deadline()), std::size_t{0});
}

void ConnectAccepted(const engine::io::Sockaddr& address, engine::SingleUseEvent& accepted) {
    engine::io::Socket kept{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    kept.Connect(address, Deadline());
    EXPECT_EQ(accepted.WaitUntil(Deadline()), engine::FutureStatus::kReady);
}

UTEST(GrpcEventEngineListener, WildcardSharesOnePortAcrossAvailableAddressFamilies) {
    const bool ipv6 = HasIpv6Loopback();
    int calls = 0;
    engine::SingleUseEvent accepted;
    ListenerFixture f{[&](int, auto, bool, auto, auto*) {
        if (++calls == (ipv6 ? 2 : 1)) {
            accepted.Send();
        }
    }};
    const auto bound = f.Bind(engine::io::Sockaddr::MakeIPv4InaddrAny());
    ASSERT_GT(bound.Port(), 0);
    for (int fd : f.listening_fds) {
        EXPECT_EQ(AddressOf(fd).Port(), bound.Port());
    }
    ASSERT_TRUE(f.listener->Start().ok());
    auto v4 = engine::io::Sockaddr::MakeIPv4LoopbackAddress();
    v4.SetPort(bound.Port());
    engine::io::Socket client4{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    client4.Connect(v4, Deadline());
    if (ipv6) {
        auto v6 = engine::io::Sockaddr::MakeIPSocketAddress("::1");
        v6.SetPort(bound.Port());
        engine::io::Socket client6{engine::io::AddrDomain::kInet6, engine::io::SocketType::kStream};
        client6.Connect(v6, Deadline());
    }
    EXPECT_EQ(accepted.WaitUntil(Deadline()), engine::FutureStatus::kReady);
}

UTEST(GrpcEventEngineListener, AcceptCallbackCanDestroyTheListener) {
    engine::SingleUseEvent accepted;
    ListenerFixture f{[&](int, auto, bool, auto, auto*) {
        f.listener.reset();
        accepted.Send();
    }};
    const auto address = f.Bind();
    ASSERT_TRUE(f.listener->Start().ok());
    engine::io::Socket client{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    client.Connect(address, Deadline());
    EXPECT_EQ(accepted.WaitUntil(Deadline()), engine::FutureStatus::kReady);
    f.owner.Shutdown();
    EXPECT_EQ(f.stopped, 1);
}

UTEST(GrpcEventEngineListener, CreatedBoundAndStartedListenersAllCloseExactlyOnce) {
    for (int stage = 0; stage < 3; ++stage) {
        ListenerFixture f;
        if (stage >= 1) {
            f.Bind();
        }
        if (stage >= 2) {
            ASSERT_TRUE(f.listener->Start().ok());
        }
        const auto fds = f.listening_fds;
        f.listener->ShutdownListeningFds();
        f.listener->ShutdownListeningFds();
        EXPECT_FALSE(f.listener->Start().ok());
        f.listener.reset();
        f.owner.Shutdown();
        EXPECT_EQ(f.stopped, 1);
        for (int fd : fds) {
            EXPECT_EQ(::fcntl(fd, F_GETFD), -1);
            EXPECT_EQ(errno, EBADF);
        }
    }
}

UTEST(GrpcEventEngineListener, QueuedBurstBeyondOneAcceptBatchIsFullyDelivered) {
    constexpr int kConnections = 96;
    int accepted = 0;
    engine::SingleUseEvent complete;
    ListenerFixture f{[&](int, auto, bool, auto, auto*) {
        if (++accepted == kConnections) {
            complete.Send();
        }
    }};
    const auto address = f.Bind();
    std::vector<engine::io::Socket> clients;
    for (int i = 0; i < kConnections; ++i) {
        clients.emplace_back(engine::io::AddrDomain::kInet, engine::io::SocketType::kStream);
        clients.back().Connect(address, Deadline());
    }
    ASSERT_TRUE(f.listener->Start().ok());
    ASSERT_EQ(complete.WaitUntil(Deadline()), engine::FutureStatus::kReady);
    EXPECT_EQ(accepted, kConnections);
}

UTEST(GrpcEventEngineListener, EndpointAllocatorFailureDropsTheConnectionAndKeepsAccepting) {
    constexpr unsigned kFailures = 1;
    engine::SingleUseEvent accepted;
    ListenerFixture f{
        [&](int, auto, bool, auto, auto*) { accepted.Send(); },
        gee::ChannelArgsEndpointConfig{},
        std::make_unique<FailingFactory>(kFailures)
    };
    const auto address = f.Bind();
    ASSERT_TRUE(f.listener->Start().ok());
    ConnectDroppedByServer(address);
    ConnectAccepted(address, accepted);
}

UTEST(GrpcEventEngineListener, BindConflictReturnsStatusWithoutNotifying) {
    engine::io::Socket occupant{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    occupant.Bind(engine::io::Sockaddr::MakeIPv4LoopbackAddress());
    occupant.Listen();
    const auto& address = occupant.Getsockname();
    ListenerFixture f{
        [](int, auto, bool, auto, auto*) {},
        gee::ChannelArgsEndpointConfig{grpc_core::ChannelArgs{}.Set(GRPC_ARG_ALLOW_REUSEPORT, 0)}
    };
    int notifications = 0;
    EXPECT_FALSE(f.listener->BindWithFd({address.Data(), address.Size()}, [&](auto) { ++notifications; }).ok());
    EXPECT_EQ(notifications, 0);
}

UTEST_DEATH(GrpcEventEngineListenerDeathTest, OwnerShutdownWithLiveListenerAndEndpointFailsTheInvariant) {
    EndpointFixture f;
    UEXPECT_DEATH(f.listening.owner.Shutdown(), "Destroy every gRPC endpoint and listener before stopping");
}

UTEST(GrpcEventEngineListener, StaleUnixSocketIsReplacedAndUnlinkedAfterShutdown) {
    const auto directory = fs::blocking::TempDirectory::Create("/tmp", "userver-ee-test-");
    const auto path = directory.GetPath() + "/listener.sock";
    const auto address = engine::io::Sockaddr::MakeUnixSocketAddress(path);
    {
        engine::io::Socket stale{engine::io::AddrDomain::kUnix, engine::io::SocketType::kStream};
        stale.Bind(address);
    }
    engine::SingleUseEvent accepted;
    ListenerFixture f{[&](int, auto, bool, auto, auto*) { accepted.Send(); }};
    f.Bind(address);
    ASSERT_TRUE(f.listener->Start().ok());
    engine::io::Socket client{engine::io::AddrDomain::kUnix, engine::io::SocketType::kStream};
    client.Connect(address, Deadline());
    EXPECT_EQ(accepted.WaitUntil(Deadline()), engine::FutureStatus::kReady);
    f.listener.reset();
    f.owner.Shutdown();
    struct stat status {};
    EXPECT_EQ(::lstat(path.c_str(), &status), -1);
    EXPECT_EQ(errno, ENOENT);
}

UTEST(GrpcEventEngineListener, BindDoesNotReplaceARegularFile) {
    const auto directory = fs::blocking::TempDirectory::Create("/tmp", "userver-ee-test-");
    const auto path = directory.GetPath() + "/regular";
    fs::blocking::RewriteFileContents(path, "keep");
    const auto address = engine::io::Sockaddr::MakeUnixSocketAddress(path);
    ListenerFixture f;
    int notifications = 0;
    EXPECT_FALSE(f.listener->BindWithFd({address.Data(), address.Size()}, [&](auto) { ++notifications; }).ok());
    EXPECT_EQ(notifications, 0);
    struct stat status {};
    ASSERT_EQ(::lstat(path.c_str(), &status), 0);
    EXPECT_TRUE(S_ISREG(status.st_mode));
    EXPECT_EQ(status.st_size, 4);
}

}  // namespace
}  // namespace ugrpc::impl::event_engine::tests

USERVER_NAMESPACE_END
