#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cstddef>
#include <memory>
#include <utility>
#include <variant>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <grpc/event_engine/memory_allocator.h>
#include <grpc/event_engine/slice_buffer.h>
#include <src/core/lib/event_engine/posix.h>

#include <userver/utils/impl/intrusive_ref_counter_one.hpp>

#include <ugrpc/impl/event_engine/closure.hpp>
#include <ugrpc/impl/event_engine/engine.hpp>
#include <ugrpc/impl/event_engine/nonblocking_socket.hpp>
#include <ugrpc/impl/event_engine/tcp_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

inline constexpr std::size_t kMaxWriteIovec = std::min<std::size_t>(260, IOV_MAX);

class Epoll1EventHandle;
class Epoll1Poller;

struct RetryIo {};
struct WaitIo {};
using IoResult = std::variant<y_absl::Status, RetryIo, WaitIo>;

struct IoState {
    std::atomic<bool> in_progress{false};
};

class PosixEndpointImpl final : public utils::impl::IntrusiveRefCounterOne<PosixEndpointImpl> {
public:
    using Callback = y_absl::AnyInvocable<void(y_absl::Status)>;
    using ReleaseCallback = y_absl::AnyInvocable<void(y_absl::StatusOr<int>)>;
    using ReadArgs = grpc_event_engine::experimental::EventEngine::Endpoint::ReadArgs;

    PosixEndpointImpl(
        NonblockingSocket socket,
        Epoll1Poller& poller,
        std::shared_ptr<PosixEventEngine> engine,
        grpc_event_engine::experimental::MemoryAllocator allocator,
        const PosixTcpOptions& options
    );
    ~PosixEndpointImpl();

    static y_absl::Status ShutdownStatus() { return y_absl::UnavailableError("Endpoint closing"); }

    bool Read(Callback on_read, grpc_event_engine::experimental::SliceBuffer* buffer, const ReadArgs* args);
    bool Write(Callback on_writable, grpc_event_engine::experimental::SliceBuffer* data);
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetPeerAddress() const {
        return peer_address_;
    }
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetLocalAddress() const {
        return local_address_;
    }
    int GetWrappedFd() const { return sock_.Fd(); }
    void MaybeShutdown(y_absl::Status why, ReleaseCallback on_release_fd);

private:
    void NotifyOnRead();
    void NotifyOnWrite();
    void ScheduleWrite();
    template <void (PosixEndpointImpl::*Handler)(y_absl::Status)>
    void Transfer(y_absl::Status status) noexcept;
    void MaybeMakeReadSlices();
    void AddToEstimate(std::size_t bytes);
    void FinishEstimate();
    auto MakeCompletion(IoState& io, Callback callback, y_absl::Status status);
    bool TcpDoRead(y_absl::Status& status);
    IoResult TcpFlush(grpc_event_engine::experimental::SliceBuffer& outgoing_buffer, std::size_t& outgoing_byte_idx);
    void HandleRead(y_absl::Status status);
    void HandleWrite(y_absl::Status status);

    std::shared_ptr<PosixEventEngine> engine_;
    NonblockingSocket sock_;
    grpc_event_engine::experimental::MemoryAllocator memory_allocator_;
    grpc_event_engine::experimental::SliceBuffer last_read_buffer_;
    double target_length_;
    double bytes_read_this_round_{0};
    const int max_read_buffer_size_;
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress peer_address_;
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress local_address_;
    std::atomic<bool> shutdown_{false};
    IoState read_;
    IoState write_;
    grpc_event_engine::experimental::SliceBuffer* incoming_buffer_{nullptr};
    grpc_event_engine::experimental::SliceBuffer* outgoing_buffer_{nullptr};
    std::size_t outgoing_byte_idx_{0};
    Callback read_cb_;
    Callback write_cb_;
    bool is_first_read_{true};
    int inq_{1};
    std::size_t min_progress_size_{1};
    PosixEngineClosure on_read_;
    PosixEngineClosure on_write_;
    ReleaseCallback on_release_fd_;
    Epoll1EventHandle* handle_;
};

class PosixEndpoint final : public grpc_event_engine::experimental::PosixEndpointWithFdSupport {
public:
    PosixEndpoint(
        NonblockingSocket socket,
        Epoll1Poller& poller,
        std::shared_ptr<PosixEventEngine> engine,
        grpc_event_engine::experimental::MemoryAllocator allocator,
        const PosixTcpOptions& options = {}
    )
        : impl_(utils::impl::MakeIntrusiveOne<
                PosixEndpointImpl>(std::move(socket), poller, std::move(engine), std::move(allocator), options))
    {}
    ~PosixEndpoint() override { impl_->MaybeShutdown(PosixEndpointImpl::ShutdownStatus(), {}); }

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
        [[maybe_unused]] const WriteArgs* args
    ) override {
        return impl_->Write(std::move(on_writable), data);
    }
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetPeerAddress() const override {
        return impl_->GetPeerAddress();
    }
    const grpc_event_engine::experimental::EventEngine::ResolvedAddress& GetLocalAddress() const override {
        return impl_->GetLocalAddress();
    }
    int GetWrappedFd() override { return impl_->GetWrappedFd(); }
    bool CanTrackErrors() override { return false; }
    void Shutdown(y_absl::AnyInvocable<void(y_absl::StatusOr<int>)> on_release_fd) override {
        impl_->MaybeShutdown(PosixEndpointImpl::ShutdownStatus(), std::move(on_release_fd));
    }

private:
    boost::intrusive_ptr<PosixEndpointImpl> impl_;
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
