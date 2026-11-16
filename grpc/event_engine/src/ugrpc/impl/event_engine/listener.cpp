#include <ugrpc/impl/event_engine/listener.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>
#include <y_absl/strings/str_cat.h>
#include <y_absl/strings/string_view.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <exception>
#include <fstream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include <sys/socket.h>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <src/core/lib/event_engine/posix_engine/tcp_socket_utils.h>
#include <src/core/lib/event_engine/tcp_socket_utils.h>

#include <userver/engine/async.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/intrusive_ref_counter_one.hpp>

#include <engine/task/task_processor.hpp>
#include <ugrpc/impl/event_engine/endpoint.hpp>
#include <ugrpc/impl/event_engine/executor.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

constexpr std::size_t kAcceptBatch = 32;
constexpr std::chrono::seconds kFileDescriptorLimitRetryDelay{1};

y_absl::Status SocketError(y_absl::string_view operation) {
    return y_absl::FailedPreconditionError(y_absl::StrCat(operation, ": ", std::strerror(errno)));
}

void UnlinkUnixSocketPath(
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
    engine::TaskProcessor& task_processor
) {
    auto& blocking_task_processor = task_processor.GetBlockingTaskProcessor();
    if (&engine::current_task::GetTaskProcessor() == &blocking_task_processor) {
        grpc_event_engine::experimental::UnlinkIfUnixDomainSocket(addr);
        return;
    }
    UINVARIANT(!Executor::IsInCallback(), "Shut down Unix listeners outside gRPC callbacks");
    const engine::TaskCancellationBlocker cancellation_blocker;
    engine::CriticalAsyncNoTracing(blocking_task_processor, [&addr] {
        grpc_event_engine::experimental::UnlinkIfUnixDomainSocket(addr);
    }).Get();
}

y_absl::StatusOr<NonblockingSocket> CreateDualStackSocket(
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
    DSMode& dsmode
) {
    int family = addr.address()->sa_family;
    if (family == AF_INET6) {
        std::optional<NonblockingSocket> sock;
        if (grpc_event_engine::experimental::PosixSocketWrapper::IsIpv6LoopbackAvailable()) {
            sock = NonblockingSocket::Create(AF_INET6);
        } else {
            errno = EAFNOSUPPORT;
        }
        if (sock && sock->SetSocketDualStack()) {
            dsmode = DSMode::kDualStack;
            return std::move(*sock);
        }
        if (!grpc_event_engine::experimental::ResolvedAddressIsV4Mapped(addr, nullptr)) {
            if (!sock) {
                return SocketError("socket");
            }
            dsmode = DSMode::kIpv6;
            return std::move(*sock);
        }
        family = AF_INET;
    }
    dsmode = family == AF_INET ? DSMode::kIpv4 : DSMode::kNone;
    auto sock = NonblockingSocket::Create(family);
    if (!sock) {
        return SocketError("socket");
    }
    return std::move(*sock);
}

int GetMaxAcceptQueueSize() {
    std::ifstream somaxconn{"/proc/sys/net/core/somaxconn"};
    int max_accept_queue_size = 0;
    if (somaxconn >> max_accept_queue_size && max_accept_queue_size > 0) {
        return max_accept_queue_size;
    }
    return SOMAXCONN;
}

y_absl::Status PrepareSocket(const PosixTcpOptions& options, ListenerSocket& socket) {
    socket.port = 0;
    if (socket.addr.address()->sa_family != AF_UNIX) {
        if (options.allow_reuse_port) {
            if (auto status = socket.sock.SetSocketReusePort(1); !status.ok()) {
                return status;
            }
        }
        if (auto status = socket.sock.SetSocketLowLatency(1); !status.ok()) {
            return status;
        }
        if (auto status = socket.sock.SetSocketReuseAddr(1); !status.ok()) {
            return status;
        }
        socket.sock.TrySetSocketTcpUserTimeout(options);
        if (options.tcp_receive_buffer_size > 0) {
            if (auto status = socket.sock.SetSocketRcvBuf(options.tcp_receive_buffer_size); !status.ok()) {
                return status;
            }
        }
    }
    if (auto status = socket.sock.Bind(socket.addr); !status.ok()) {
        return y_absl::FailedPreconditionError(y_absl::StrCat(
            "Error in bind for address '",
            grpc_event_engine::experimental::ResolvedAddressToString(socket.addr).value_or("<unparsable>"),
            "': ",
            status.message()
        ));
    }
    if (auto status = socket.sock.Listen(GetMaxAcceptQueueSize()); !status.ok()) {
        return y_absl::FailedPreconditionError(y_absl::StrCat("Error in listen: ", status.message()));
    }
    const auto sockname = socket.sock.LocalAddress();
    if (!sockname.ok()) {
        return y_absl::FailedPreconditionError(y_absl::StrCat("Error in getsockname: ", sockname.status().message()));
    }
    socket.port = grpc_event_engine::experimental::ResolvedAddressGetPort(*sockname);
    return y_absl::OkStatus();
}

y_absl::StatusOr<ListenerSocket> CreateAndPrepareListenerSocket(
    const PosixTcpOptions& options,
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr
) {
    DSMode dsmode = DSMode::kNone;
    auto sock = CreateDualStackSocket(addr, dsmode);
    if (!sock.ok()) {
        return sock.status();
    }
    grpc_event_engine::experimental::EventEngine::ResolvedAddress addr4_copy;
    const bool use_addr4 =
        dsmode == DSMode::kIpv4 && grpc_event_engine::experimental::ResolvedAddressIsV4Mapped(addr, &addr4_copy);
    ListenerSocket socket{std::move(*sock), 0, dsmode, use_addr4 ? addr4_copy : addr};
    if (auto status = PrepareSocket(options, socket); !status.ok()) {
        return status;
    }
    UASSERT(socket.port > 0);
    return socket;
}

template <typename Container>
y_absl::StatusOr<int> ListenerContainerAddWildcardAddresses(
    Container& listener_sockets,
    const PosixTcpOptions& options,
    int requested_port
) {
    auto wild4 = grpc_event_engine::experimental::ResolvedAddressMakeWild4(requested_port);
    const auto wild6 = grpc_event_engine::experimental::ResolvedAddressMakeWild6(requested_port);
    int assigned_port = 0;
    auto v6_sock = CreateAndPrepareListenerSocket(options, wild6);
    if (v6_sock.ok()) {
        const auto dsmode = v6_sock->dsmode;
        requested_port = v6_sock->port;
        assigned_port = v6_sock->port;
        listener_sockets.Append(std::move(*v6_sock));
        if (dsmode == DSMode::kDualStack || dsmode == DSMode::kIpv4) {
            return assigned_port;
        }
    }
    grpc_event_engine::experimental::ResolvedAddressSetPort(wild4, requested_port);
    auto v4_sock = CreateAndPrepareListenerSocket(options, wild4);
    if (v4_sock.ok()) {
        assigned_port = v4_sock->port;
        listener_sockets.Append(std::move(*v4_sock));
    }
    if (assigned_port > 0) {
        if (!v6_sock.ok()) {
            LOG_INFO() << "Failed to add :: listener, the environment may not support IPv6: " << v6_sock.status();
        }
        if (!v4_sock.ok()) {
            LOG_INFO() << "Failed to add 0.0.0.0 listener, the environment may not support IPv4: " << v4_sock.status();
        }
        return assigned_port;
    }
    return y_absl::FailedPreconditionError(
        y_absl::StrCat("Failed to add any wildcard listeners: ", v6_sock.status().message(), v4_sock.status().message())
    );
}

y_absl::StatusOr<std::string> PeerName(const AcceptedSocket& accepted) {
    if (accepted.addr.address()->sa_family != AF_UNIX) {
        return grpc_event_engine::experimental::ResolvedAddressToURI(accepted.addr);
    }
    const auto peer_address = accepted.sock.PeerAddress();
    if (!peer_address.ok()) {
        return peer_address.status();
    }
    return grpc_event_engine::experimental::ResolvedAddressToURI(*peer_address);
}

}  // namespace

PosixEngineListenerImpl::PosixEngineListenerImpl(
    PosixAcceptCallback on_accept,
    y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
    const grpc_event_engine::experimental::EndpointConfig& config,
    std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory,
    Epoll1Poller& poller,
    std::shared_ptr<PosixEventEngine> engine
)
    : poller_(poller),
      options_(event_engine::TcpOptionsFromEndpointConfig(config)),
      engine_(std::move(engine)),
      acceptors_(*this),
      on_accept_(std::move(on_accept)),
      on_shutdown_(std::move(on_shutdown)),
      memory_allocator_factory_(std::move(memory_allocator_factory))
{}

y_absl::StatusOr<int> PosixEngineListenerImpl::Bind(
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
    OnPosixBindNewFdCallback on_bind_new_fd
) {
    if (started_ || shutdown_) {
        return y_absl::FailedPreconditionError(
            started_ ? "Listener is already started, ports can no longer be bound"
                     : "Listener is shut down, ports can no longer be bound"
        );
    }
    acceptors_.UpdateOnAppendCallback(std::move(on_bind_new_fd));
    grpc_event_engine::experimental::EventEngine::ResolvedAddress res_addr = addr;
    grpc_event_engine::experimental::EventEngine::ResolvedAddress addr6_v4mapped;
    int requested_port = grpc_event_engine::experimental::ResolvedAddressGetPort(res_addr);
    UASSERT(addr.size() <= grpc_event_engine::experimental::EventEngine::ResolvedAddress::MAX_SIZE_BYTES);
    grpc_event_engine::experimental::UnlinkIfUnixDomainSocket(addr);

    for (auto it = acceptors_.begin(); requested_port == 0 && it != acceptors_.end(); ++it) {
        if (const int used_port = (*it)->Socket().port; used_port > 0) {
            requested_port = used_port;
            grpc_event_engine::experimental::ResolvedAddressSetPort(res_addr, requested_port);
            break;
        }
    }

    try {
        if (const auto wildcard_port = grpc_event_engine::experimental::ResolvedAddressIsWildcard(res_addr)) {
            return ListenerContainerAddWildcardAddresses(acceptors_, options_, *wildcard_port);
        }
        const bool v4_mapped = grpc_event_engine::experimental::ResolvedAddressToV4Mapped(res_addr, &addr6_v4mapped);
        auto result = CreateAndPrepareListenerSocket(options_, v4_mapped ? addr6_v4mapped : res_addr);
        if (!result.ok()) {
            return result.status();
        }
        const int port = result->port;
        acceptors_.Append(std::move(*result));
        return port;
    } catch (const std::exception& error) {
        return y_absl::FailedPreconditionError(error.what());
    }
}

void PosixEngineListenerImpl::ListenerAsyncAcceptors::Append(ListenerSocket socket) {
    auto acceptor = utils::impl::MakeIntrusiveOne<
        AsyncConnectionAcceptor>(listener_.engine_, listener_.shared_from_this(), std::move(socket));
    const int fd = acceptor->Socket().sock.Fd();
    acceptors_.push_back(std::move(acceptor));
    if (on_append_) {
        on_append_(fd);
    }
}

PosixEngineListenerImpl::AsyncConnectionAcceptor::AsyncConnectionAcceptor(
    std::shared_ptr<PosixEventEngine> engine,
    std::shared_ptr<PosixEngineListenerImpl> listener,
    ListenerSocket socket
)
    : engine_(std::move(engine)),
      listener_(std::move(listener)),
      socket_(std::move(socket)),
      handle_(listener_->poller_.CreateHandle(socket_.sock.Fd())),
      notify_on_accept_([this](y_absl::Status status) {
          engine_->Post([this, status = std::move(status)]() mutable { NotifyOnAccept(std::move(status)); });
      })
{}

PosixEngineListenerImpl::AsyncConnectionAcceptor::~AsyncConnectionAcceptor() {
    int released_fd = engine::io::kInvalidFd;
    handle_->OrphanHandle(&released_fd, "");
    UASSERT(released_fd == socket_.sock.Fd());
}

void PosixEngineListenerImpl::AsyncConnectionAcceptor::Start() {
    intrusive_ptr_add_ref(this);
    handle_->NotifyOnRead(&notify_on_accept_);
}

void PosixEngineListenerImpl::AsyncConnectionAcceptor::NotifyOnAccept(y_absl::Status status) {
    if (!status.ok()) {
        intrusive_ptr_release(this);
        return;
    }
    std::size_t accepted_count = 0;
    for (;;) {
        if (handle_->IsHandleShutdown()) {
            intrusive_ptr_release(this);
            return;
        }
        if (accepted_count == kAcceptBatch) {
            accepted_count = 0;
            engine::Yield();
            continue;
        }
        auto accepted = socket_.sock.Accept4();
        if (!accepted) {
            switch (errno) {
                case EMFILE:
                    LOG_LIMITED_ERROR() << "File descriptor limit reached. Retrying.";
                    if (!retry_timer_armed_.exchange(true)) {
                        std::ignore = engine_->RunAfter(
                            kFileDescriptorLimitRetryDelay,
                            [self = boost::intrusive_ptr<AsyncConnectionAcceptor>{this}] {
                                self->retry_timer_armed_.store(false);
                                if (!self->handle_->IsHandleShutdown()) {
                                    self->handle_->SetReadable();
                                }
                            }
                        );
                    }
                    handle_->NotifyOnRead(&notify_on_accept_);
                    return;
                case EAGAIN:
                case ECONNABORTED:
                    handle_->NotifyOnRead(&notify_on_accept_);
                    return;
                default:
                    LOG_ERROR()
                        << "Stopping accepts on listening fd " << socket_.sock.Fd()
                        << ". Failed accept4: " << std::strerror(errno);
                    intrusive_ptr_release(this);
                    return;
            }
        }
        ++accepted_count;
        const auto peer_name = PeerName(*accepted);
        if (!peer_name.ok()) {
            LOG_ERROR()
                << "Stopping accepts on listening fd " << socket_.sock.Fd()
                << ". Failed to resolve the peer: " << peer_name.status();
            intrusive_ptr_release(this);
            return;
        }
        std::unique_ptr<PosixEndpoint> endpoint;
        grpc_event_engine::experimental::MemoryAllocator allocator;
        try {
            endpoint = std::make_unique<PosixEndpoint>(
                std::move(accepted->sock),
                listener_->poller_,
                listener_->engine_,
                listener_->memory_allocator_factory_
                    ->CreateMemoryAllocator(y_absl::StrCat("endpoint-tcp-server-connection: ", *peer_name)),
                listener_->options_
            );
            allocator =
                listener_->memory_allocator_factory_
                    ->CreateMemoryAllocator(y_absl::StrCat("on-accept-tcp-server-connection: ", *peer_name));
        } catch (const std::exception& error) {
            LOG_LIMITED_ERROR() << "Dropping an accepted connection from " << *peer_name << ": " << error;
            continue;
        }
        Executor::RunInCallbackScope([&] {
            listener_->on_accept_(handle_->WrappedFd(), std::move(endpoint), false, std::move(allocator), nullptr);
        });
    }
}

void PosixEngineListenerImpl::AsyncConnectionAcceptor::Shutdown() {
    handle_->ShutdownHandle(y_absl::InternalError("Shutting down acceptor"));
    if (socket_.addr.address()->sa_family == AF_UNIX) {
        engine_->Post([self = boost::intrusive_ptr<AsyncConnectionAcceptor>{this}] {
            UnlinkUnixSocketPath(self->socket_.addr, self->engine_->GetTaskProcessor());
        });
    }
}

y_absl::Status PosixEngineListenerImpl::Start() {
    if (started_ || shutdown_) {
        return y_absl::FailedPreconditionError(started_ ? "Listener is already started" : "Listener is shut down");
    }
    started_ = true;
    for (const auto& acceptor : acceptors_) {
        acceptor->Start();
    }
    return y_absl::OkStatus();
}

void PosixEngineListenerImpl::TriggerShutdown() {
    shutdown_ = true;
    for (const auto& acceptor : acceptors_.Take()) {
        acceptor->Shutdown();
    }
}

PosixEngineListenerImpl::~PosixEngineListenerImpl() {
    Executor::RunInCallbackScope([this] {
        on_accept_ = nullptr;
        memory_allocator_factory_.reset();
        if (on_shutdown_ != nullptr) {
            on_shutdown_(y_absl::OkStatus());
        }
    });
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
