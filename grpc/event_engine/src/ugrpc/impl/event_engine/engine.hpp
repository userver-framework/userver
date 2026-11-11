#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>

#include <cstddef>
#include <memory>

#include <src/core/lib/event_engine/posix.h>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

class PosixEventEngine final : public grpc_event_engine::experimental::PosixEventEngineWithFdSupport {
public:
    using AnyInvocable = y_absl::AnyInvocable<void()>;

    explicit PosixEventEngine(engine::TaskProcessor& task_processor);
    ~PosixEventEngine() override;
    PosixEventEngine(const PosixEventEngine&) = delete;
    PosixEventEngine& operator=(const PosixEventEngine&) = delete;

    engine::TaskProcessor& GetTaskProcessor() const noexcept;
    bool IsStopping() const noexcept;
    bool IsStopped() const noexcept;

    void Run(Closure* closure) override;
    void Run(AnyInvocable closure) override;
    void Post(AnyInvocable closure) noexcept;
    TaskHandle RunAfter(Duration when, Closure* closure) override;
    TaskHandle RunAfter(Duration when, AnyInvocable closure) override;
    bool Cancel(TaskHandle handle) override;
    bool IsWorkerThread() override;

    y_absl::StatusOr<std::unique_ptr<grpc_event_engine::experimental::PosixListenerWithFdSupport>> CreatePosixListener(
        PosixAcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory
    ) override;
    y_absl::StatusOr<std::unique_ptr<grpc_event_engine::experimental::EventEngine::Listener>> CreateListener(
        grpc_event_engine::experimental::EventEngine::Listener::AcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory
    ) override;
    std::unique_ptr<grpc_event_engine::experimental::PosixEndpointWithFdSupport> CreatePosixEndpointFromFd(
        int fd,
        const grpc_event_engine::experimental::EndpointConfig& config,
        grpc_event_engine::experimental::MemoryAllocator allocator
    ) override;
    ConnectionHandle Connect(
        OnConnectCallback on_connect,
        const ResolvedAddress& address,
        const grpc_event_engine::experimental::EndpointConfig& config,
        grpc_event_engine::experimental::MemoryAllocator allocator,
        Duration timeout
    ) override;
    bool CancelConnect(ConnectionHandle handle) override;
    y_absl::StatusOr<std::unique_ptr<DNSResolver>> GetDNSResolver(const DNSResolver::ResolverOptions& options) override;

private:
    friend class PosixEventEngineOwner;
    void Shutdown() noexcept;
    struct Impl;
    static constexpr std::size_t kImplSize = 2048;
    utils::FastPimpl<Impl, kImplSize, concurrent::impl::kDestructiveInterferenceSize> impl_;
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
