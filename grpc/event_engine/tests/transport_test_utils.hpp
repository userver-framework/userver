#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <src/core/lib/event_engine/channel_args_endpoint_config.h>
#include <src/core/lib/event_engine/memory_allocator_factory.h>
#include <src/core/lib/resource_quota/resource_quota.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>

#include <ugrpc/impl/event_engine/executor.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine::tests {

namespace gee = grpc_event_engine::experimental;

inline engine::Deadline Deadline() { return engine::Deadline::FromDuration(utest::kMaxTestWaitTime); }

inline engine::io::Sockaddr AddressOf(int fd) {
    sockaddr_storage storage{};
    socklen_t size = sizeof(storage);
    UINVARIANT(::getsockname(fd, reinterpret_cast<sockaddr*>(&storage), &size) == 0, "getsockname failed");
    return engine::io::Sockaddr{reinterpret_cast<sockaddr*>(&storage)};
}

struct ListenerFixture {
    PosixEventEngineOwner owner{engine::current_task::GetTaskProcessor()};
    grpc_core::ResourceQuotaRefPtr quota{grpc_core::MakeResourceQuota("event-engine-listener-test")};
    std::atomic<int> stopped{0};
    std::vector<int> listening_fds;
    std::unique_ptr<gee::PosixListenerWithFdSupport> listener;

    explicit ListenerFixture(
        gee::PosixEventEngineWithFdSupport::PosixAcceptCallback accept = [](int, auto, bool, auto, auto*) {},
        const gee::EndpointConfig& config = gee::ChannelArgsEndpointConfig{},
        std::unique_ptr<gee::MemoryAllocatorFactory> factory = nullptr
    ) {
        if (!factory) {
            factory = std::make_unique<gee::MemoryQuotaBasedMemoryAllocatorFactory>(quota->memory_quota());
        }
        auto result = GetPosixEventEngine(owner)->CreatePosixListener(
            std::move(accept),
            [this](y_absl::Status status) {
                EXPECT_TRUE(status.ok());
                EXPECT_TRUE(Executor::IsInCallback());
                ++stopped;
            },
            config,
            std::move(factory)
        );
        UINVARIANT(result.ok(), result.status().ToString());
        listener = std::move(*result);
    }

    ~ListenerFixture() {
        listener.reset();
        owner.Shutdown();
    }

    engine::io::Sockaddr Bind(engine::io::Sockaddr address = engine::io::Sockaddr::MakeIPv4LoopbackAddress()) {
        auto actual = address;
        const auto result = listener->BindWithFd({address.Data(), address.Size()}, [&](y_absl::StatusOr<int> fd) {
            UINVARIANT(fd.ok(), fd.status().ToString());
            listening_fds.push_back(*fd);
            actual = AddressOf(*fd);
        });
        UINVARIANT(result.ok(), result.status().ToString());
        if (actual.HasPort()) {
            EXPECT_EQ(*result, actual.Port());
            address.SetPort(actual.Port());
        }
        return address;
    }
};

struct Completion {
    gee::SliceBuffer buffer;
    engine::Promise<y_absl::Status> promise;
    engine::Future<y_absl::Status> result{promise.get_future()};
    std::atomic<int> calls{0};

    void Done(y_absl::Status status) {
        EXPECT_TRUE(engine::current_task::IsTaskProcessorThread());
        EXPECT_TRUE(Executor::IsInCallback());
        if (calls.fetch_add(1) == 0) {
            promise.set_value(std::move(status));
        } else {
            ADD_FAILURE() << "Duplicate completion";
        }
    }

    y_absl::Status Wait() {
        UINVARIANT(result.wait_for(utest::kMaxTestWaitTime) == engine::FutureStatus::kReady, "I/O timed out");
        return result.get();
    }
};

inline std::string Bytes(gee::SliceBuffer& buffer) {
    std::string result;
    for (std::size_t i = 0; i < buffer.Count(); ++i) {
        result.append(reinterpret_cast<const char*>(buffer[i].data()), buffer[i].size());
    }
    return result;
}

struct EndpointFixture {
    engine::Promise<std::unique_ptr<grpc_event_engine::experimental::EventEngine::Endpoint>> incoming;
    engine::Future<std::unique_ptr<grpc_event_engine::experimental::EventEngine::Endpoint>> accepted{
        incoming.get_future()
    };
    ListenerFixture listening;
    engine::io::Socket peer{engine::io::AddrDomain::kInet, engine::io::SocketType::kStream};
    std::unique_ptr<grpc_event_engine::experimental::EventEngine::Endpoint> endpoint;

    explicit EndpointFixture(const gee::EndpointConfig& config = gee::ChannelArgsEndpointConfig{})
        : listening(
              [this](int, auto ep, bool, auto, auto*) {
                  EXPECT_TRUE(Executor::IsInCallback());
                  incoming.set_value(std::move(ep));
              },
              config
          )
    {
        const auto address = listening.Bind();
        UINVARIANT(listening.listener->Start().ok(), "Listener start failed");
        peer.Connect(address, Deadline());
        UINVARIANT(accepted.wait_for(utest::kMaxTestWaitTime) == engine::FutureStatus::kReady, "Accept timed out");
        endpoint = accepted.get();
    }

    ~EndpointFixture() {
        endpoint.reset();
        listening.listener.reset();
        listening.owner.Shutdown();
    }

    gee::PosixEndpointWithFdSupport& Posix() { return static_cast<gee::PosixEndpointWithFdSupport&>(*endpoint); }

    void Send(const std::string& bytes) {
        UINVARIANT(peer.SendAll(bytes.data(), bytes.size(), Deadline()) == bytes.size(), "Peer send failed");
    }

    y_absl::Status Read(Completion& completion, std::int64_t hint = 1) {
        const grpc_event_engine::experimental::EventEngine::Endpoint::ReadArgs args{hint};
        if (endpoint
                ->Read([&completion](auto status) { completion.Done(std::move(status)); }, &completion.buffer, &args))
        {
            EXPECT_EQ(completion.calls, 0);
            return y_absl::OkStatus();
        }
        return completion.Wait();
    }

    std::string ReadBytes(std::size_t size, std::int64_t hint = 1) {
        std::string result;
        while (result.size() < size) {
            Completion read;
            const auto status = Read(read, hint);
            UINVARIANT(status.ok(), status.ToString());
            result += Bytes(read.buffer);
        }
        return result;
    }
};

}  // namespace ugrpc::impl::event_engine::tests

USERVER_NAMESPACE_END
