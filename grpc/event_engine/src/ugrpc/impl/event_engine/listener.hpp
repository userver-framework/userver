#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>

#include <atomic>
#include <list>
#include <memory>
#include <utility>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <src/core/lib/event_engine/posix.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/intrusive_ref_counter_one.hpp>

#include <ugrpc/impl/event_engine/closure.hpp>
#include <ugrpc/impl/event_engine/engine.hpp>
#include <ugrpc/impl/event_engine/ev_epoll1_linux.hpp>
#include <ugrpc/impl/event_engine/nonblocking_socket.hpp>
#include <ugrpc/impl/event_engine/tcp_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

enum class DSMode { kNone, kIpv4, kIpv6, kDualStack };

struct ListenerSocket final {
    NonblockingSocket sock;
    int port{0};
    DSMode dsmode{DSMode::kNone};
    grpc_event_engine::experimental::EventEngine::ResolvedAddress addr;
};

class PosixEngineListenerImpl final : public std::enable_shared_from_this<PosixEngineListenerImpl> {
public:
    using PosixAcceptCallback = grpc_event_engine::experimental::PosixEventEngineWithFdSupport::PosixAcceptCallback;
    using OnPosixBindNewFdCallback =
        grpc_event_engine::experimental::PosixListenerWithFdSupport::OnPosixBindNewFdCallback;

    PosixEngineListenerImpl(
        PosixAcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory,
        Epoll1Poller& poller,
        std::shared_ptr<PosixEventEngine> engine
    );
    ~PosixEngineListenerImpl();

    y_absl::StatusOr<int> Bind(
        const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
        OnPosixBindNewFdCallback on_bind_new_fd
    );
    y_absl::Status Start();
    void TriggerShutdown();

private:
    class AsyncConnectionAcceptor final : public utils::impl::IntrusiveRefCounterOne<AsyncConnectionAcceptor> {
    public:
        AsyncConnectionAcceptor(
            std::shared_ptr<PosixEventEngine> engine,
            std::shared_ptr<PosixEngineListenerImpl> listener,
            ListenerSocket socket
        );
        ~AsyncConnectionAcceptor();

        void Start();
        void NotifyOnAccept(y_absl::Status status);
        void Shutdown();
        const ListenerSocket& Socket() const noexcept { return socket_; }

    private:
        std::shared_ptr<PosixEventEngine> engine_;
        std::shared_ptr<PosixEngineListenerImpl> listener_;
        ListenerSocket socket_;
        Epoll1EventHandle* handle_;
        PosixEngineClosure notify_on_accept_;
        std::atomic<bool> retry_timer_armed_{false};
    };

    using Acceptors = std::list<boost::intrusive_ptr<AsyncConnectionAcceptor>>;

    class ListenerAsyncAcceptors final {
    public:
        explicit ListenerAsyncAcceptors(PosixEngineListenerImpl& listener) noexcept : listener_(listener) {}

        void UpdateOnAppendCallback(OnPosixBindNewFdCallback on_append) { on_append_ = std::move(on_append); }
        void Append(ListenerSocket socket);
        Acceptors Take() noexcept { return std::exchange(acceptors_, {}); }
        Acceptors::const_iterator begin() const noexcept { return acceptors_.begin(); }
        Acceptors::const_iterator end() const noexcept { return acceptors_.end(); }

    private:
        OnPosixBindNewFdCallback on_append_;
        Acceptors acceptors_;
        PosixEngineListenerImpl& listener_;
    };

    friend class ListenerAsyncAcceptors;
    friend class AsyncConnectionAcceptor;

    Epoll1Poller& poller_;
    const PosixTcpOptions options_;
    std::shared_ptr<PosixEventEngine> engine_;
    ListenerAsyncAcceptors acceptors_;
    PosixAcceptCallback on_accept_;
    y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown_;
    bool started_{false};
    bool shutdown_{false};
    std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory_;
};

class PosixEngineListener final : public grpc_event_engine::experimental::PosixListenerWithFdSupport {
public:
    PosixEngineListener(
        grpc_event_engine::experimental::PosixEventEngineWithFdSupport::PosixAcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory,
        Epoll1Poller& poller,
        std::shared_ptr<PosixEventEngine> engine
    )
        : impl_(std::make_shared<PosixEngineListenerImpl>(
              std::move(on_accept),
              std::move(on_shutdown),
              config,
              std::move(memory_allocator_factory),
              poller,
              std::move(engine)
          ))
    {}
    ~PosixEngineListener() override { ShutdownListeningFds(); }

    y_absl::StatusOr<int> Bind(const grpc_event_engine::experimental::EventEngine::ResolvedAddress&) override {
        utils::AbortWithStacktrace("unimplemented");
    }
    y_absl::StatusOr<int> BindWithFd(
        const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
        OnPosixBindNewFdCallback on_bind_new_fd
    ) override {
        return impl_->Bind(addr, std::move(on_bind_new_fd));
    }
    y_absl::Status Start() override { return impl_->Start(); }
    void ShutdownListeningFds() override {
        if (!shutdown_.exchange(true, std::memory_order_acq_rel)) {
            impl_->TriggerShutdown();
        }
    }
    y_absl::Status HandleExternalConnection(int, int, grpc_event_engine::experimental::SliceBuffer*) override {
        utils::AbortWithStacktrace("unimplemented");
    }

private:
    std::shared_ptr<PosixEngineListenerImpl> impl_;
    std::atomic<bool> shutdown_{false};
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
