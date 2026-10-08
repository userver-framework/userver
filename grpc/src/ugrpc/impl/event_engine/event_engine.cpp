#include <ugrpc/impl/event_engine/event_engine.hpp>

#include <atomic>
#include <mutex>

#include <sys/stat.h>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <grpc/event_engine/event_engine.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

#include <contrib/libs/grpc/src/core/lib/event_engine/default_event_engine.h>
#include <contrib/libs/grpc/src/core/lib/event_engine/posix.h>

#include <ugrpc/impl/event_engine/executor.hpp>
#include <ugrpc/impl/event_engine/timer_manager.hpp>

#include <engine/impl/non_cancellable_awaiter.hpp>

USERVER_NAMESPACE_BEGIN

// clang-format off

namespace ugrpc::impl {

namespace {

template <typename Fn>
void EnsureRunInTaskProcessorThread(engine::TaskProcessor& task_processor, Fn fn) {
    if (engine::current_task::IsTaskProcessorThread()) {
        fn();
    } else {
        auto task = engine::CriticalAsyncNoTracing(task_processor, [&fn] { fn(); });
        task.BlockingWait();
    }
}

// grpc_event_engine::experimental::EventEngine::ResolvedAddress MakeResolvedAddress(const engine::io::Sockaddr&
// sockaddr ) {
//     return grpc_event_engine::experimental::EventEngine::ResolvedAddress(sockaddr.Data(), sockaddr.Size());
// }

void UnlinkIfUnixDomainSocket(const engine::io::Sockaddr& sockaddr) {
    if (engine::io::AddrDomain::kUnix == sockaddr.Domain()) {
        const std::string sockpath = sockaddr.PrimaryAddressString();

        struct stat st {};
        if (::stat(sockpath.c_str(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFSOCK) {
            ::unlink(sockpath.c_str());
        }
    }
}

std::size_t SendNoblock(engine::io::Socket& socket, grpc_event_engine::experimental::SliceBuffer& data) {
    if (0 == data.Count()) {
        return 0;
    }

    if (1 == data.Count()) {
        const auto& slice = data[0];
        if (slice.empty()) {
            return 0;
        }
        return socket.SendNoblock(slice.data(), slice.size(), {});
    }

    constexpr size_t kMaxStackSizeVector = 32;

    if (data.Count() < kMaxStackSizeVector) {
        std::array<engine::io::IoData, kMaxStackSizeVector> buffers{};

        std::size_t index = 0;
        for (std::size_t i = 0; i < data.Count(); ++i) {
            const auto& slice = data[i];

            if (slice.empty()) {
                continue;
            }

            buffers[index].data = slice.data();
            buffers[index].len = slice.size();
            ++index;
        }

        if (0 == index) {
            return 0;
        }

        return socket.SendNoblock(buffers.data(), index, {});
    }

    std::vector<engine::io::IoData> buffers;
    buffers.reserve(data.Count());

    for (std::size_t index = 0; index < data.Count(); ++index) {
        const auto& slice = data[index];
        if (slice.empty()) {
            continue;
        }

        buffers.push_back(engine::io::IoData{
            .data = slice.data(),
            .len = slice.size(),
        });
    }

    if (buffers.empty()) {
        return 0;
    }

    return socket.SendNoblock(buffers.data(), buffers.size(), {});
}

// Ref-counted implementation of the gRPC endpoint, modelled on gRPC's own
// PosixEndpointImpl (see contrib/libs/grpc/src/core/lib/event_engine/posix_engine/posix_endpoint.h).
//
// Background: the gRPC EventEngine endpoint destructor can be invoked from
// inside an `ExecCtx::Flush()` callback chain on a userver coroutine (the
// chttp2 transport drops its last ref, which inline triggers ~Endpoint via
// `grpc_endpoint_destroy`). If the endpoint destructor blocks (yields) the
// coroutine, the coroutine may migrate between worker threads, breaking
// gRPC's `thread_local ExecCtx` invariants and crashing the combiner.
//
// To make the destructor non-yielding we split the endpoint into a thin
// wrapper (PosixEndpoint) and a ref-counted implementation (this class).
// The wrapper holds a boost::intrusive_ptr<PosixEndpointImpl>; in-flight
// ReadAsync/WriteAsync lambdas also capture a boost::intrusive_ptr.
// PosixEndpoint::~PosixEndpoint() calls MaybeShutdown() (non-blocking)
// and then drops its intrusive_ptr ref; the impl is destroyed when the
// last ref drops, which happens naturally when all in-flight lambdas fire.
class PosixEndpointImpl final : public boost::intrusive_ref_counter<PosixEndpointImpl> {
public:
    PosixEndpointImpl(
        engine::TaskProcessor& task_processor,
        concurrent::BackgroundTaskStorageCore& background_task_storage,
        Executor& executor,
        engine::io::Socket&& client_socket,
        grpc_event_engine::experimental::MemoryAllocator&& memory_allocator,
        std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
    )
        : task_processor_{task_processor},
          background_task_storage_{background_task_storage},
          executor_{executor},
          client_socket_{std::move(client_socket)},
          memory_allocator_{std::move(memory_allocator)},
          event_engine_{std::move(event_engine)}
    {
        (void) executor_;

        // peer_address_ = MakeResolvedAddress(client_socket_.Getpeername());
        // local_address_ = MakeResolvedAddress(client_socket_.Getsockname());
    }

    bool Read(
        y_absl::AnyInvocable<void(y_absl::Status)> on_read,
        grpc_event_engine::experimental::SliceBuffer* buffer,
        const grpc_event_engine::experimental::EventEngine::Endpoint::ReadArgs* args
    ) {
        UASSERT(on_read);
        UASSERT(buffer != nullptr);

        const std::size_t min_progress_size = static_cast<std::size_t>(std::max<std::int64_t>(args->read_hint_bytes, 1));

        // LOG_DEBUG("endpoint {} Read, min_progress_size={}", static_cast<void*>(this), min_progress_size);

        bool expected = false;
        UINVARIANT(read_in_progress_.compare_exchange_strong(expected, true), "Endpoint::Read called while another read is still outstanding");

        if (last_read_error_) {
            // LOG_DEBUG("endpoint {} read, last_read_error", static_cast<void*>(this));
            read_in_progress_.store(false);
            // executor_.Run([on_read = std::move(on_read)]() mutable {
            //     on_read(y_absl::InternalError("Socket read last error"));
            // });
            on_read(y_absl::InternalError("Socket read last error"));
            return false;
        }

        buffer->Clear();

        constexpr std::size_t kReadSliceSize = 8 * 1024;
        buffer->AppendIndexed(grpc_event_engine::experimental::Slice(memory_allocator_.MakeSlice(kReadSliceSize)));

        std::size_t total_read = 0;

        try {
            /// fast path
            auto& slice = grpc_event_engine::experimental::internal::SliceCast<
                grpc_event_engine::experimental::MutableSlice>(buffer->MutableSliceAt(0));
            const auto read_bytes = client_socket_.RecvNoblock(slice.begin(), slice.length());
            // LOG_DEBUG("endpoint {} read (noblock), read bytes=", static_cast<void*>(this)) << read_bytes;

            if (read_bytes.has_value()) {
                if (min_progress_size <= *read_bytes) {
                    // LOG_DEBUG("endpoint {} read (noblock) done, read bytes={}", static_cast<void*>(this), *read_bytes);
                    buffer->RemoveLastNBytes(buffer->Length() - *read_bytes);
                    read_in_progress_.store(false);
                    return true;
                }

                if (0 == *read_bytes) {
                    // LOG_DEBUG("endpoint {} read (noblock) error", static_cast<void*>(this));
                    buffer->Clear();
                    last_read_error_ = true;
                    read_in_progress_.store(false);
                    // executor_.Run([on_read = std::move(on_read)]() mutable {
                    //     on_read(y_absl::CancelledError("Endpoint is shutting down"));
                    // });
                    on_read(y_absl::CancelledError("Endpoint is shutting down"));
                    return false;
                }

                total_read += *read_bytes;
            }
        } catch (const std::exception& ex) {
            // LOG_ERROR("endpoint {} read (noblock) error: ", static_cast<void*>(this)) << ex;
            buffer->Clear();
            last_read_error_ = true;
            read_in_progress_.store(false);
            // executor_.Run([on_read = std::move(on_read)]() mutable {
            //     on_read(y_absl::UnknownError("Socket read (fast) error"));
            // });
            on_read(y_absl::UnknownError("Socket read (fast) error"));
            return false;
        }

        intrusive_ptr_add_ref(this);
        ReadAsync(std::move(on_read), buffer, min_progress_size, total_read);

        return false;
    }

    bool Write(
        y_absl::AnyInvocable<void(y_absl::Status)> on_writable,
        grpc_event_engine::experimental::SliceBuffer* data,
        const grpc_event_engine::experimental::EventEngine::Endpoint::WriteArgs* args
    ) {
        UASSERT(on_writable);
        UASSERT(data != nullptr);

        (void)args;

        // LOG_DEBUG("endpoint {} Write called bytes={}", static_cast<void*>(this), data->Length());

        bool expected = false;
        UINVARIANT(write_in_progress_.compare_exchange_strong(expected, true), "Endpoint::Write called while another write is still outstanding");

        if (last_write_error_) {
            // LOG_DEBUG("endpoint {} write, last_write_error", static_cast<void*>(this));
            write_in_progress_.store(false);
            // executor_.Run([on_writable = std::move(on_writable)]() mutable {
            //     on_writable(y_absl::InternalError("Socket write last error"));
            // });
            on_writable(y_absl::InternalError("Socket write last error"));
            return false;
        }

        if (0 == data->Length()) {
            write_in_progress_.store(false);
            return true;
        }

        try {
            /// fast path
            const auto total_size = data->Length();
            const auto write_bytes = impl::SendNoblock(client_socket_, *data);
            // LOG_DEBUG("endpoint {} write (noblock), write bytes={}", static_cast<void*>(this), write_bytes);

            if (write_bytes == total_size) {
                // LOG_DEBUG("endpoint {} write (noblock) done, write bytes={}", static_cast<void*>(this), write_bytes);
                write_in_progress_.store(false);
                return true;
            }

            if (0 < write_bytes) {
                grpc_event_engine::experimental::SliceBuffer fast_write_buffer;
                data->MoveFirstNBytesIntoSliceBuffer(write_bytes, fast_write_buffer);
            }
        } catch (const std::exception& ex) {
            // LOG_ERROR("endpoint {} socket write (noblock) error: ", static_cast<void*>(this)) << ex;
            last_write_error_ = true;
            write_in_progress_.store(false);
            // executor_.Run([on_writable = std::move(on_writable)]() mutable {
            //     on_writable(y_absl::UnknownError("Socket write (fast) error"));
            // });
            on_writable(y_absl::UnknownError("Socket write (fast) error"));
            return false;
        }

        intrusive_ptr_add_ref(this);
        WriteAsync(std::move(on_writable), data);

        return false;
    }

    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetPeerAddress() const noexcept {
        return peer_address_;
    }

    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetLocalAddress() const noexcept {
        return local_address_;
    }

    int GetWrappedFd() const noexcept { return client_socket_.Fd(); }

    void MaybeShutdown(y_absl::AnyInvocable<void(y_absl::StatusOr<int>)> on_release_fd) {
        on_release_fd_ = std::move(on_release_fd);
    }

    ~PosixEndpointImpl() {
        // LOG_DEBUG("endpoint {} destroy started", static_cast<void*>(this));

        UASSERT(!read_in_progress_);
        UASSERT(!write_in_progress_);

        background_task_storage_.Detach(engine::CriticalAsyncNoTracing(
            task_processor_,
            [client_socket = std::move(client_socket_)]() mutable noexcept {
                try {
                    // LOG_DEBUG("client socket ({}) closing... ", client_socket.Fd());

                    client_socket.Close();

                    // LOG_DEBUG("client socket closed");
                } catch (const std::exception& ex) {
                    // LOG_ERROR("client socket close error: ") << ex;
                }
            }
        ));

        // LOG_DEBUG("endpoint {} destroy complete", static_cast<void*>(this));
    }

private:
    void ReadAsync(
        y_absl::AnyInvocable<void(y_absl::Status)> on_read,
        grpc_event_engine::experimental::SliceBuffer* buffer,
        std::size_t min_progress_size,
        std::size_t total_read
    ) {
        // LOG_DEBUG("endpoint {} ReadAsync, min_progress_size={}, total_read={}", static_cast<void*>(this), min_progress_size, total_read);

        engine::impl::AppendNonCancellableAwaiter(
            client_socket_.GetReadableBase().GetAwaitableToken().GetAwaitable(utils::impl::InternalTag{}),
            [this,
             on_read = std::move(on_read),
             buffer,
             min_progress_size,
             total_read]() mutable noexcept {
                try {
                    auto& slice = grpc_event_engine::experimental::internal::SliceCast<
                        grpc_event_engine::experimental::MutableSlice>(buffer->MutableSliceAt(0));

                    // LOG_DEBUG("endpoint {} ReadAsync, reading...", static_cast<void*>(this));
                    const auto read_bytes = client_socket_.RecvNoblock(slice.begin() + total_read, slice.length() - total_read);
                    // LOG_DEBUG("endpoint {} ReadAsync, remaining={}, read bytes=", static_cast<void*>(this), min_progress_size - total_read) << read_bytes;

                    if (read_bytes.has_value() && 0 < *read_bytes) {
                        total_read += *read_bytes;

                        if (min_progress_size <= total_read) {
                            buffer->RemoveLastNBytes(buffer->Length() - total_read);
                            read_in_progress_.store(false);
                            on_read(y_absl::OkStatus());
                            intrusive_ptr_release(this);
                            return;
                        }

                        ReadAsync(std::move(on_read), buffer, min_progress_size, total_read);
                        return;
                    }
                } catch (const std::exception& ex) {
                    // LOG_DEBUG("endpoint {} ReadAsync, exception: ", static_cast<void*>(this)) << ex;
                }

                // LOG_DEBUG("endpoint {} ReadAsync, read error", static_cast<void*>(this));

                last_read_error_ = true;

                if (0 < total_read) {
                    buffer->RemoveLastNBytes(buffer->Length() - total_read);
                    read_in_progress_.store(false);
                    on_read(y_absl::OkStatus());
                    intrusive_ptr_release(this);
                    return;
                }

                buffer->Clear();
                read_in_progress_.store(false);

                on_read(y_absl::UnknownError("Socket read (async) error"));
                intrusive_ptr_release(this);
            }
        );
    }

    void WriteAsync(
        y_absl::AnyInvocable<void(y_absl::Status)> on_writable,
        grpc_event_engine::experimental::SliceBuffer* data
    ) {
        // LOG_DEBUG("endpoint {} WriteAsync", static_cast<void*>(this));

        engine::impl::AppendNonCancellableAwaiter(
            client_socket_.GetWritableBase().GetAwaitableToken().GetAwaitable(utils::impl::InternalTag{}),
            [this, on_writable = std::move(on_writable), data]() mutable noexcept {
                try {
                    // LOG_DEBUG("endpoint {} WriteAsync, writing...", static_cast<void*>(this));
                    const auto write_bytes = impl::SendNoblock(client_socket_, *data);
                    // LOG_DEBUG("endpoint {} WriteAsync, size={}, write bytes={}", static_cast<void*>(this), data->Length(), write_bytes);

                    if (0 < write_bytes) {
                        grpc_event_engine::experimental::SliceBuffer fast_write_buffer;
                        data->MoveFirstNBytesIntoSliceBuffer(write_bytes, fast_write_buffer);

                        if (0 == data->Length()) {
                            write_in_progress_.store(false);
                            on_writable(y_absl::OkStatus());
                            intrusive_ptr_release(this);
                            return;
                        }

                        WriteAsync(std::move(on_writable), data);
                        return;
                    }
                } catch (const std::exception& ex) {
                    // LOG_DEBUG("endpoint {} WriteAsync, exception: ", static_cast<void*>(this)) << ex;
                }

                // LOG_DEBUG("endpoint {} WriteAsync, write error", static_cast<void*>(this));

                last_write_error_ = true;
                write_in_progress_.store(false);

                on_writable(y_absl::UnknownError("Socket write (async) error"));
                intrusive_ptr_release(this);
            }
        );
    }

    engine::TaskProcessor& task_processor_;

    concurrent::BackgroundTaskStorageCore& background_task_storage_;
    Executor& executor_;

    engine::io::Socket client_socket_;
    grpc_event_engine::experimental::EventEngine::ResolvedAddress peer_address_;
    grpc_event_engine::experimental::EventEngine::ResolvedAddress local_address_;

    grpc_event_engine::experimental::MemoryAllocator memory_allocator_;

    std::atomic<bool> read_in_progress_{false};
    std::atomic<bool> write_in_progress_{false};

    bool last_read_error_{false};
    bool last_write_error_{false};

    y_absl::AnyInvocable<void(y_absl::StatusOr<int>)> on_release_fd_;

    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine_;
};

class PosixEndpoint final : public grpc_event_engine::experimental::PosixEndpointWithFdSupport {
public:
    PosixEndpoint(
        engine::TaskProcessor& task_processor,
        concurrent::BackgroundTaskStorageCore& background_task_storage,
        Executor& executor,
        engine::io::Socket&& client_socket,
        grpc_event_engine::experimental::MemoryAllocator&& memory_allocator,
        std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
    )
        : impl_{utils::make_intrusive_ptr<PosixEndpointImpl>(
              task_processor,
              background_task_storage,
              executor,
              std::move(client_socket),
              std::move(memory_allocator),
              std::move(event_engine)
          )}
    {}

    ~PosixEndpoint() override {
        if (!shutdown_.exchange(true, std::memory_order_acq_rel)) {
            impl_->MaybeShutdown(nullptr);
        }
    }

    bool Read(
        y_absl::AnyInvocable<void(y_absl::Status)> on_read,
        grpc_event_engine::experimental::SliceBuffer* buffer,
        const ReadArgs* args
    ) override {
        return impl_->Read(std::move(on_read), buffer, args);
    }

    bool Write(
        y_absl::AnyInvocable<void(y_absl::Status)> on_writable,
        grpc_event_engine::experimental::SliceBuffer* data,
        const WriteArgs* args
    ) override {
        return impl_->Write(std::move(on_writable), data, args);
    }

    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetPeerAddress() const override {
        return impl_->GetPeerAddress();
    }

    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetLocalAddress() const override {
        return impl_->GetLocalAddress();
    }

    int GetWrappedFd() override { return impl_->GetWrappedFd(); }

    bool CanTrackErrors() override { return false; }

    void Shutdown(y_absl::AnyInvocable<void(y_absl::StatusOr<int> release_fd)> on_release_fd) override {
        if (!shutdown_.exchange(true, std::memory_order_acq_rel)) {
            impl_->MaybeShutdown(std::move(on_release_fd));
        }
    }

private:
    boost::intrusive_ptr<PosixEndpointImpl> impl_;
    std::atomic<bool> shutdown_{false};
};

// Implementation of the gRPC listener, modelled on gRPC's own
// PosixEngineListenerImpl. Owned by PosixEngineListener via
// boost::intrusive_ptr; accept loops and per-accept tasks capture
// boost::intrusive_ptr<Self>. This keeps the impl alive until all
// in-flight async work finishes.
class PosixEngineListenerImpl final : public boost::intrusive_ref_counter<PosixEngineListenerImpl> {
public:
    PosixEngineListenerImpl(
        engine::TaskProcessor& task_processor,
        concurrent::BackgroundTaskStorageCore& background_task_storage,
        Executor& executor,
        grpc_event_engine::experimental::PosixEventEngineWithFdSupport::PosixAcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory,
        std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
    )
        : task_processor_{task_processor},
          background_task_storage_{background_task_storage},
          executor_{executor},
          on_accept_{std::move(on_accept)},
          on_shutdown_{std::move(on_shutdown)},
          memory_allocator_factory_{std::move(memory_allocator_factory)},
          event_engine_{std::move(event_engine)}
    {
        // LOG_DEBUG("listener {} created", static_cast<void*>(this));
        (void) config;
    }

    ~PosixEngineListenerImpl() {
        // LOG_DEBUG("listener {} destroy started", static_cast<void*>(this));

        UASSERT(socket_listener_tasks_.empty());

        if (on_shutdown_) {
            on_shutdown_(y_absl::OkStatus());
        }

        // LOG_DEBUG("listener {} destroy complete", static_cast<void*>(this));
    }

    y_absl::StatusOr<int> BindWithFd(
        const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
        grpc_event_engine::experimental::PosixListenerWithFdSupport::OnPosixBindNewFdCallback on_bind_new_fd
    ) {
        UASSERT(on_bind_new_fd);
        UASSERT(!starting_);

        // LOG_DEBUG("listener {}, bind started", static_cast<void*>(this));

        engine::io::Sockaddr sockaddr{addr.address()};
        UnlinkIfUnixDomainSocket(sockaddr);

        engine::io::Socket listen_socket{sockaddr.Domain(), engine::io::SocketType::kStream};
        listen_socket.Bind(sockaddr);
        listen_socket.Listen(/*backlog*/ 1024);
        const int listener_fd = listen_socket.Fd();

        {
            std::lock_guard lk{mu_};
            listen_sockets_.push_back(std::move(listen_socket));
        }

        // `on_bind_new_fd` should be run synchronously
        on_bind_new_fd(listener_fd);

        // LOG_DEBUG("listener {}, bind complete", static_cast<void*>(this));

        return 1;
    }

    y_absl::Status Start() {
        // LOG_DEBUG("listener {} starting...", static_cast<void*>(this));

        starting_ = true;

        std::vector<engine::io::Socket> listen_sockets;
        {
            std::lock_guard lk{mu_};
            listen_sockets = std::move(listen_sockets_);
        }
        for (auto& listen_socket : listen_sockets) {
            intrusive_ptr_add_ref(this);
            socket_listener_tasks_.push_back(engine::CriticalAsyncNoTracing(
                task_processor_,
                [this, listen_socket = std::move(listen_socket)]() mutable noexcept {
                    // LOG_DEBUG("listener {} task started", static_cast<void*>(this));

                    while (!engine::current_task::ShouldCancel()) {
                        try {
                            AcceptConnection(listen_socket);
                        } catch (const engine::io::IoCancelled& ex) {
                            // LOG_INFO("listener {} task IoCancelled: ", static_cast<void*>(this)) << ex;
                            break;
                        } catch (const std::exception& ex) {
                            // LOG_ERROR("listener {} task error: ", static_cast<void*>(this)) << ex;
                            break;
                        }
                    }

                    intrusive_ptr_release(this);

                    try {
                        // LOG_DEBUG("listen socket ({}) closing... ", listen_socket.Fd());

                        listen_socket.Close();

                        // LOG_DEBUG("listen socket closed");
                    } catch (const std::exception& ex) {
                        // LOG_ERROR("listen socket close error: ") << ex;
                    }

                    // LOG_DEBUG("listener task complete");
                }
            ));
        }

        started_ = true;

        // LOG_DEBUG("listener {} started", static_cast<void*>(this));

        return y_absl::OkStatus();
    }

    // Non-blocking: just requests cancellation of accept loops. The actual
    // shutdown (waiting for loops to exit, draining per-accept tasks, firing
    // on_shutdown) happens asynchronously when all shared_ptr refs drop.
    // The wrapper's shutdown_ flag ensures this is called at most once.
    void TriggerShutdown() {
        UASSERT(started_);

        // LOG_DEBUG("listener {} shutdown started", static_cast<void*>(this));

        // Accept loops check ShouldCancel() each iteration and IoCancelled
        // is thrown from a blocking Accept call on cancel.
        for (auto& task : socket_listener_tasks_) {
            task.RequestCancel();
            background_task_storage_.Detach(std::move(task));
        }

        socket_listener_tasks_.clear();

        // LOG_DEBUG("listener {} shutdown complete", static_cast<void*>(this));
    }

private:
    void AcceptConnection(engine::io::Socket& listen_socket) {
        const auto listener_fd = listen_socket.Fd();

        // LOG_DEBUG("listener {} task, accept connection...", static_cast<void*>(this));
        engine::io::Socket client_socket = listen_socket.Accept({});
        // LOG_DEBUG("listener {} task, connection accepted (fd={})", static_cast<void*>(this), client_socket.Fd());

        if (!engine::current_task::ShouldCancel()) {
            intrusive_ptr_add_ref(this);
            background_task_storage_.Detach(engine::CriticalAsyncNoTracing(
                task_processor_,
                [this, listener_fd, client_socket = std::move(client_socket)]() mutable noexcept {
                    // LOG_DEBUG("listener {} accept connection task started (fd={})", static_cast<void*>(this), client_socket.Fd());

                    try {
                        auto endpoint = std::make_unique<PosixEndpoint>(
                            task_processor_,
                            background_task_storage_,
                            executor_,
                            std::move(client_socket),
                            memory_allocator_factory_->CreateMemoryAllocator("endpoint: peername=xxx"),
                            event_engine_
                        );

                        // LOG_DEBUG("listener {} on_accept started", static_cast<void*>(this));
                        on_accept_(
                            listener_fd,
                            std::move(endpoint),
                            /*is_external*/ false,
                            memory_allocator_factory_->CreateMemoryAllocator("on-accept: peername=xxx"),
                            /*pending_data=*/nullptr
                        );
                        // LOG_DEBUG("listener {} on_accept completed", static_cast<void*>(this));
                    } catch (const std::exception& ex) {
                        // LOG_ERROR("listener {} accept connection task error: ", static_cast<void*>(this)) << ex;
                    }

                    intrusive_ptr_release(this);
                }
            ));
        }
    }

    engine::TaskProcessor& task_processor_;

    concurrent::BackgroundTaskStorageCore& background_task_storage_;
    Executor& executor_;

    std::mutex mu_;
    std::vector<engine::io::Socket> listen_sockets_;

    std::vector<engine::TaskWithResult<void>> socket_listener_tasks_;

    grpc_event_engine::experimental::PosixEventEngineWithFdSupport::PosixAcceptCallback on_accept_;
    y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown_;

    std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory_;

    std::atomic<bool> starting_{false};
    std::atomic<bool> started_{false};

    std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine_;
};

class PosixEngineListener final : public grpc_event_engine::experimental::PosixListenerWithFdSupport {
public:
    PosixEngineListener(
        engine::TaskProcessor& task_processor,
        concurrent::BackgroundTaskStorageCore& background_task_storage,
        Executor& executor,
        grpc_event_engine::experimental::PosixEventEngineWithFdSupport::PosixAcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory,
        std::shared_ptr<grpc_event_engine::experimental::EventEngine> event_engine
    )
        : impl_{utils::make_intrusive_ptr<PosixEngineListenerImpl>(
              task_processor,
              background_task_storage,
              executor,
              std::move(on_accept),
              std::move(on_shutdown),
              config,
              std::move(memory_allocator_factory),
              std::move(event_engine)
          )}
    {}

    ~PosixEngineListener() override {
        if (!shutdown_.exchange(true, std::memory_order_acq_rel)) {
            impl_->TriggerShutdown();
        }
    }

    y_absl::StatusOr<int> BindWithFd(
        const grpc_event_engine::experimental::EventEngine::ResolvedAddress& addr,
        OnPosixBindNewFdCallback on_bind_new_fd
    ) override {
        return impl_->BindWithFd(addr, std::move(on_bind_new_fd));
    }

    y_absl::StatusOr<int> Bind(const grpc_event_engine::experimental::EventEngine::ResolvedAddress& /*addr*/) override {
        utils::AbortWithStacktrace("unimplemented");
    }

    y_absl::Status Start() override { return impl_->Start(); }

    y_absl::Status HandleExternalConnection(
        int /*listener_fd*/,
        int /*fd*/,
        grpc_event_engine::experimental::SliceBuffer* /*pending_data*/
    ) override {
        utils::AbortWithStacktrace("unimplemented");
    }

    void ShutdownListeningFds() override {
        if (!shutdown_.exchange(true, std::memory_order_acq_rel)) {
            impl_->TriggerShutdown();
        }
    }

private:
    boost::intrusive_ptr<PosixEngineListenerImpl> impl_;
    std::atomic<bool> shutdown_{false};
};

class PosixEventEngine final : public grpc_event_engine::experimental::PosixEventEngineWithFdSupport {
public:
    explicit PosixEventEngine(engine::TaskProcessor& task_processor)
        : task_processor_{task_processor},
          executor_{task_processor_, background_task_storage_},
          timer_manager_{task_processor_}
    {
        // LOG_DEBUG("engine {} created", static_cast<void*>(this));
    }

    ~PosixEventEngine() override {
        // LOG_DEBUG("engine {} destroy started", static_cast<void*>(this));

        // Per gRPC's EventEngine contract (see event_engine.h:385-392), the
        // engine must have no active responsibilities at destruction time:
        // all endpoints and listeners must already be shut down. PosixEndpoint
        // / PosixEngineListener cleanup chains each hold a shared_ptr<EventEngine>
        // via their PosixEndpointImpl / PosixEngineListenerImpl; this
        // destructor only runs after they have all dropped their refs.

        impl::EnsureRunInTaskProcessorThread(task_processor_, [this] {
            background_task_storage_.CancelAndWait();
            // LOG_INFO("engine {} destroy, all background tasks complete", static_cast<void*>(this));
        });

        // LOG_DEBUG("engine {} destroy complete", static_cast<void*>(this));
    }

    std::unique_ptr<grpc_event_engine::experimental::PosixEndpointWithFdSupport> CreatePosixEndpointFromFd(
        int /*fd*/,
        const grpc_event_engine::experimental::EndpointConfig& /*config*/,
        grpc_event_engine::experimental::MemoryAllocator /*memory_allocator*/
    ) override {
        utils::AbortWithStacktrace("unimplemented");
    }

    y_absl::StatusOr<std::unique_ptr<grpc_event_engine::experimental::PosixListenerWithFdSupport>> CreatePosixListener(
        PosixAcceptCallback on_accept,
        y_absl::AnyInvocable<void(y_absl::Status)> on_shutdown,
        const grpc_event_engine::experimental::EndpointConfig& config,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> memory_allocator_factory
    ) override {
        // LOG_DEBUG("engine {} create listener", static_cast<void*>(this));
        return std::make_unique<PosixEngineListener>(
            task_processor_,
            background_task_storage_,
            executor_,
            std::move(on_accept),
            std::move(on_shutdown),
            config,
            std::move(memory_allocator_factory),
            shared_from_this()
        );
    }

    y_absl::StatusOr<std::unique_ptr<Listener>> CreateListener(
        Listener::AcceptCallback /*on_accept*/,
        y_absl::AnyInvocable<void(y_absl::Status)> /*on_shutdown*/,
        const grpc_event_engine::experimental::EndpointConfig& /*config*/,
        std::unique_ptr<grpc_event_engine::experimental::MemoryAllocatorFactory> /*memory_allocator_factory*/
    ) override {
        utils::AbortWithStacktrace("unimplemented");
    }

    ConnectionHandle Connect(
        OnConnectCallback /*on_connect*/,
        const ResolvedAddress& /*addr*/,
        const grpc_event_engine::experimental::EndpointConfig& /*args*/,
        grpc_event_engine::experimental::MemoryAllocator /*memory_allocator*/,
        Duration /*timeout*/
    ) override {
        utils::AbortWithStacktrace("unimplemented");
    }

    bool CancelConnect(ConnectionHandle /*handle*/) override { utils::AbortWithStacktrace("unimplemented"); }

    bool IsWorkerThread() override { utils::AbortWithStacktrace("unimplemented"); }

    y_absl::StatusOr<std::unique_ptr<DNSResolver>> GetDNSResolver(const DNSResolver::ResolverOptions&) override {
        utils::AbortWithStacktrace("unimplemented");
    }

    void Run(Closure* closure) override {
        UASSERT(closure != nullptr);

        // LOG_DEBUG("engine {} Run(closure={})", static_cast<void*>(this), static_cast<void*>(closure));

        executor_.Run([closure] { closure->Run(); });
    }

    void Run(y_absl::AnyInvocable<void()> closure) override {
        UASSERT(!!closure);

        // LOG_DEBUG("engine {} Run(closure)", static_cast<void*>(this));

        executor_.Run(std::move(closure));
    }

    TaskHandle RunAfter(Duration when, Closure* closure) override {
        UASSERT(closure != nullptr);

        // LOG_DEBUG("engine {} RunAfter(when={}, *closure)", static_cast<void*>(this), when.count());

        return RunAfterImpl(when, [closure] { closure->Run(); });
    }

    TaskHandle RunAfter(Duration when, y_absl::AnyInvocable<void()> closure) override {
        UASSERT(!!closure);

        // LOG_DEBUG("engine {} RunAfter(when={}, closure)", static_cast<void*>(this), when.count());

        return RunAfterImpl(when, std::move(closure));
    }

    bool Cancel(TaskHandle handle) override {
        // LOG_DEBUG("engine {} Cancel", static_cast<void*>(this));

        return timer_manager_.TimerCancel(handle);
    }

private:
    TaskHandle RunAfterImpl(Duration when, y_absl::AnyInvocable<void()> closure) {
        return timer_manager_.TimerInit(when, std::move(closure));
    }

    engine::TaskProcessor& task_processor_;

    concurrent::BackgroundTaskStorageCore background_task_storage_;
    Executor executor_;

    TimerManager timer_manager_;
};

std::shared_ptr<grpc_event_engine::experimental::EventEngine> kEventEngine;

}  // namespace

void SetEventEngineFactory() {
    // LOG_DEBUG("SetEventEngineFactory");

    engine::TaskProcessor& task_processor = engine::current_task::GetTaskProcessor();

    grpc_event_engine::experimental::SetEventEngineFactory([&task_processor] {
        return std::make_unique<PosixEventEngine>(task_processor);
    });

    kEventEngine = grpc_event_engine::experimental::GetDefaultEventEngine();

    // LOG_DEBUG("SetEventEngineFactory complete");
}

void ShutdownDefaultEventEngine() {
    // LOG_DEBUG("ShutdownDefaultEventEngine");

    // EventEngineFactoryReset replacement, due to synchronization problem
    // grpc_event_engine::experimental::SetEventEngineFactory([] {
    //     return nullptr;
    // });
    grpc_event_engine::experimental::EventEngineFactoryReset();

    kEventEngine.reset();

    // LOG_DEBUG("ShutdownDefaultEventEngine complete");
}

}  // namespace ugrpc::impl

// clang-format on

USERVER_NAMESPACE_END
