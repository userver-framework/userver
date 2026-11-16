#include <ugrpc/impl/event_engine/endpoint.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>
#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>
#include <y_absl/strings/str_cat.h>
#include <y_absl/strings/string_view.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <variant>

#include <sys/uio.h>

#include <grpc/event_engine/memory_request.h>
#include <grpc/event_engine/slice.h>
#include <grpc/event_engine/slice_buffer.h>

#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/intrusive_ref_counter_one.hpp>

#include <ugrpc/impl/event_engine/engine.hpp>
#include <ugrpc/impl/event_engine/ev_epoll1_linux.hpp>
#include <ugrpc/impl/event_engine/executor.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {
namespace {

constexpr std::size_t kSmallAlloc = 8 * 1024;
constexpr std::size_t kBigAlloc = 64 * 1024;
constexpr std::size_t kLargeSliceThreshold = kSmallAlloc + kSmallAlloc / 2;
constexpr std::size_t kMaxReadIovec = 64;
constexpr std::size_t kMaxInlineIoAttempts = 16;
constexpr std::size_t kIoAttemptsBeforeYield = 16;
constexpr double kReadEstimateGrowthThreshold = 0.8;
constexpr double kReadEstimateGrowthFactor = 2.0;
constexpr double kReadEstimatePreviousWeight = 0.99;
constexpr double kReadEstimateCurrentWeight = 0.01;
static_assert(kMaxInlineIoAttempts != 0 && kIoAttemptsBeforeYield != 0);

template <typename TryIo>
IoResult TryIoInline(TryIo&& try_io, std::size_t max_attempts = kMaxInlineIoAttempts) {
    for (std::size_t attempts = 0; attempts < max_attempts; ++attempts) {
        auto result = try_io();
        if (!std::holds_alternative<RetryIo>(result)) {
            return result;
        }
    }
    return RetryIo{};
}

template <typename TryIo>
IoResult TryIoOnWorker(TryIo&& try_io) {
    for (;;) {
        auto result = TryIoInline(try_io, kIoAttemptsBeforeYield);
        if (!std::holds_alternative<RetryIo>(result)) {
            return result;
        }
        engine::Yield();
    }
}

grpc_event_engine::experimental::EventEngine::ResolvedAddress AddressOrEmpty(
    y_absl::StatusOr<grpc_event_engine::experimental::EventEngine::ResolvedAddress> address
) {
    if (!address.ok()) {
        LOG_LIMITED_WARNING() << "Accepted endpoint address is unavailable: " << address.status();
        return {};
    }
    return *address;
}

std::optional<std::size_t> TcpSend(
    const NonblockingSocket& sock,
    grpc_event_engine::experimental::SliceBuffer& outgoing_buffer,
    std::size_t outgoing_byte_idx
) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<iovec, kMaxWriteIovec> iov;
    std::size_t iov_size = 0;
    for (std::size_t outgoing_slice_idx = 0; outgoing_slice_idx < outgoing_buffer.Count() && iov_size < iov.size();
         ++outgoing_slice_idx)
    {
        const auto& slice = outgoing_buffer[outgoing_slice_idx];
        const auto offset = outgoing_slice_idx == 0 ? outgoing_byte_idx : 0;
        UASSERT(offset <= slice.size());
        if (offset != slice.size()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            iov[iov_size++] = {const_cast<std::uint8_t*>(slice.data()) + offset, slice.size() - offset};
        }
    }
    UASSERT(iov_size != 0);
    return sock.Send(std::span<const iovec>{iov.data(), iov_size});
}

}  // namespace

template <void (PosixEndpointImpl::*Handler)(y_absl::Status)>
void PosixEndpointImpl::Transfer(y_absl::Status status) noexcept {
    static_assert(Handler != nullptr);
    constexpr bool kAddReference = false;
    engine_->Post([self = boost::intrusive_ptr<PosixEndpointImpl>{this, kAddReference},
                   status = std::move(status)]() mutable { (self.get()->*Handler)(std::move(status)); });
}

PosixEndpointImpl::PosixEndpointImpl(
    NonblockingSocket socket,
    Epoll1Poller& poller,
    std::shared_ptr<PosixEventEngine> engine,
    grpc_event_engine::experimental::MemoryAllocator allocator,
    const PosixTcpOptions& options
)
    : engine_(std::move(engine)),
      sock_(std::move(socket)),
      memory_allocator_(std::move(allocator)),
      target_length_(options.tcp_read_chunk_size),
      max_read_buffer_size_(options.tcp_max_read_buffer_size),
      peer_address_(AddressOrEmpty(sock_.PeerAddress())),
      local_address_(AddressOrEmpty(sock_.LocalAddress())),
      on_read_([this](y_absl::Status status) { Transfer<&PosixEndpointImpl::HandleRead>(std::move(status)); }),
      on_write_([this](y_absl::Status status) { Transfer<&PosixEndpointImpl::HandleWrite>(std::move(status)); }),
      handle_(poller.CreateHandle(sock_.Fd()))
{}

void PosixEndpointImpl::MaybeShutdown(y_absl::Status why, ReleaseCallback on_release_fd) {
    auto self = boost::intrusive_ptr<PosixEndpointImpl>{this};
    if (!shutdown_.exchange(true)) {
        on_release_fd_ = std::move(on_release_fd);
        handle_->ShutdownHandle(std::move(why));
    } else if (on_release_fd) {
        engine_->Run([self = std::move(self), on_release_fd = std::move(on_release_fd)]() mutable {
            on_release_fd(y_absl::FailedPreconditionError(
                "PosixEndpoint shutdown was already requested; only the first call can release the fd"
            ));
        });
    }
}

PosixEndpointImpl::~PosixEndpointImpl() {
    UASSERT(!read_.in_progress.load() && !write_.in_progress.load());
    UASSERT(!read_cb_ && !write_cb_ && !incoming_buffer_ && !outgoing_buffer_);
    int release_fd = engine::io::kInvalidFd;
    handle_->OrphanHandle(&release_fd, "");
    UASSERT(release_fd == sock_.Fd());
    if (on_release_fd_) {
        engine_->Run([callback = std::move(on_release_fd_), fd = std::move(sock_).Release()]() mutable { callback(fd); }
        );
    }
}

void PosixEndpointImpl::NotifyOnRead() {
    intrusive_ptr_add_ref(this);
    handle_->NotifyOnRead(&on_read_);
}

void PosixEndpointImpl::NotifyOnWrite() {
    intrusive_ptr_add_ref(this);
    handle_->NotifyOnWrite(&on_write_);
}

void PosixEndpointImpl::ScheduleWrite() {
    intrusive_ptr_add_ref(this);
    on_write_.SetStatus(y_absl::OkStatus());
    on_write_.Run();
}

void PosixEndpointImpl::MaybeMakeReadSlices() {
    if (incoming_buffer_->Length() < std::max<std::size_t>(min_progress_size_, 1)) {
        std::size_t allocate_length = min_progress_size_;
        const auto target_length = static_cast<std::size_t>(target_length_);
        if (target_length > allocate_length) {
            allocate_length = target_length;
        }
        std::size_t extra_wanted = std::max<std::size_t>(1, allocate_length - incoming_buffer_->Length());
        const std::size_t preferred = extra_wanted >= kLargeSliceThreshold ? kBigAlloc : kSmallAlloc;
        while (extra_wanted > 0 && incoming_buffer_->Count() < kMaxReadIovec) {
            auto slice = grpc_event_engine::experimental::Slice(
                memory_allocator_.MakeSlice(grpc_event_engine::experimental::MemoryRequest{kSmallAlloc, preferred})
            );
            const auto granted = slice.size();
            UASSERT(granted >= kSmallAlloc && granted <= preferred);
            incoming_buffer_->AppendIndexed(std::move(slice));
            extra_wanted -= std::min(extra_wanted, granted);
            if (granted < preferred) {
                break;
            }
        }
    }
}

void PosixEndpointImpl::AddToEstimate(std::size_t bytes) { bytes_read_this_round_ += static_cast<double>(bytes); }

void PosixEndpointImpl::FinishEstimate() {
    if (bytes_read_this_round_ > target_length_ * kReadEstimateGrowthThreshold) {
        target_length_ = std::max(kReadEstimateGrowthFactor * target_length_, bytes_read_this_round_);
    } else {
        target_length_ =
            kReadEstimatePreviousWeight * target_length_ + kReadEstimateCurrentWeight * bytes_read_this_round_;
    }
    if (max_read_buffer_size_ >= 0) {
        target_length_ = std::min(target_length_, static_cast<double>(max_read_buffer_size_));
    }
    bytes_read_this_round_ = 0;
}

auto PosixEndpointImpl::MakeCompletion(IoState& io, Callback callback, y_absl::Status status) {
    return
        [self = boost::intrusive_ptr<PosixEndpointImpl>{this},
         &io,
         callback = std::move(callback),
         status = std::move(status)]() mutable {
            if (self->shutdown_.load()) {
                status = ShutdownStatus();
            }
            io.in_progress.store(false);
            callback(std::move(status));
        };
}

bool PosixEndpointImpl::TcpDoRead(y_absl::Status& status) {
    std::array<iovec, kMaxReadIovec> iov;
    std::size_t total_read_bytes = 0;
    std::size_t iov_len = std::min<std::size_t>(kMaxReadIovec, incoming_buffer_->Count());
    std::size_t slice_idx = iov_len;
    for (std::size_t i = 0; i < iov_len; ++i) {
        auto& slice = grpc_event_engine::experimental::internal::SliceCast<
            grpc_event_engine::experimental::MutableSlice>(incoming_buffer_->MutableSliceAt(i));
        iov[i] = {slice.begin(), slice.size()};
    }

    UASSERT(incoming_buffer_->Length() != 0);
    UASSERT(min_progress_size_ > 0);

    do {
        inq_ = 1;

        std::optional<std::size_t> res;
        try {
            res = sock_.Recv(std::span<const iovec>{iov.data(), iov_len});
        } catch (const engine::io::IoSystemError& error) {
            if (total_read_bytes >= 1) {
                break;
            }
            incoming_buffer_->Clear();
            status = y_absl::UnavailableError(y_absl::StrCat("recvmsg:", error.Code().message()));
            return true;
        }

        if (!res) {
            if (total_read_bytes > 0) {
                break;
            }
            FinishEstimate();
            inq_ = 0;
            return false;
        }
        const std::size_t read_bytes = *res;
        if (read_bytes == 0 && total_read_bytes >= 1) {
            break;
        }
        if (read_bytes == 0) {
            incoming_buffer_->Clear();
            status = y_absl::UnavailableError("Socket closed");
            return true;
        }
        AddToEstimate(read_bytes);
        UASSERT(read_bytes <= incoming_buffer_->Length() - total_read_bytes);

        total_read_bytes += read_bytes;
        if (total_read_bytes == incoming_buffer_->Length()) {
            break;
        }

        std::size_t remaining = read_bytes;
        std::size_t j = 0;
        for (std::size_t i = 0; i < iov_len; ++i) {
            if (remaining >= iov[i].iov_len) {
                remaining -= iov[i].iov_len;
                continue;
            }
            if (remaining > 0) {
                iov[j] = {static_cast<char*>(iov[i].iov_base) + remaining, iov[i].iov_len - remaining};
                remaining = 0;
            } else {
                iov[j] = iov[i];
            }
            ++j;
        }
        iov_len = j;
        while (iov_len < kMaxReadIovec && slice_idx < incoming_buffer_->Count()) {
            auto& slice = grpc_event_engine::experimental::internal::SliceCast<
                grpc_event_engine::experimental::MutableSlice>(incoming_buffer_->MutableSliceAt(slice_idx));
            iov[iov_len] = {slice.begin(), slice.size()};
            ++iov_len;
            ++slice_idx;
        }
    } while (true);

    UASSERT(total_read_bytes > 0);
    status = y_absl::OkStatus();
    if (total_read_bytes < incoming_buffer_->Length()) {
        incoming_buffer_
            ->MoveLastNBytesIntoSliceBuffer(incoming_buffer_->Length() - total_read_bytes, last_read_buffer_);
    }
    return true;
}

IoResult PosixEndpointImpl::TcpFlush(
    grpc_event_engine::experimental::SliceBuffer& outgoing_buffer,
    std::size_t& outgoing_byte_idx
) {
    if (shutdown_.load()) {
        return ShutdownStatus();
    }
    if (outgoing_buffer.Length() == 0) {
        return y_absl::OkStatus();
    }
    try {
        const auto sent_length = TcpSend(sock_, outgoing_buffer, outgoing_byte_idx);
        if (!sent_length) {
            return WaitIo{};
        }
        if (*sent_length == 0) {
            return y_absl::UnavailableError("The socket made no write progress");
        }
        UASSERT(*sent_length <= outgoing_buffer.Length() - outgoing_byte_idx);
        if (*sent_length == outgoing_buffer.Length() - outgoing_byte_idx) {
            return y_absl::OkStatus();
        }
        outgoing_byte_idx += *sent_length;
        while (outgoing_byte_idx >= outgoing_buffer[0].size()) {
            auto slice = outgoing_buffer.TakeFirst();
            outgoing_byte_idx -= slice.size();
        }
        return RetryIo{};
    } catch (const std::exception& error) {
        return y_absl::UnavailableError(error.what());
    }
}

void PosixEndpointImpl::HandleRead(y_absl::Status status) {
    UASSERT(incoming_buffer_ && read_cb_);
    if (status.ok()) {
        bool completed = true;
        try {
            MaybeMakeReadSlices();
            completed = TcpDoRead(status);
        } catch (const std::exception& error) {
            status = y_absl::ResourceExhaustedError(error.what());
            incoming_buffer_->Clear();
            last_read_buffer_.Clear();
        }
        if (!completed) {
            NotifyOnRead();
            return;
        }
    } else {
        incoming_buffer_->Clear();
        last_read_buffer_.Clear();
    }
    incoming_buffer_ = nullptr;
    Executor::RunInCallbackScope([&] {
        auto callback = MakeCompletion(read_, std::exchange(read_cb_, {}), std::move(status));
        callback();
    });
}

void PosixEndpointImpl::HandleWrite(y_absl::Status status) {
    UASSERT(outgoing_buffer_ && write_cb_);
    if (status.ok()) {
        auto result = TryIoOnWorker([&] { return TcpFlush(*outgoing_buffer_, outgoing_byte_idx_); });
        if (std::holds_alternative<WaitIo>(result)) {
            NotifyOnWrite();
            return;
        }
        status = std::get<y_absl::Status>(std::move(result));
    }
    handle_->ReleaseWriteInterest();
    outgoing_buffer_->Clear();
    outgoing_buffer_ = nullptr;
    Executor::RunInCallbackScope([&] {
        auto callback = MakeCompletion(write_, std::exchange(write_cb_, {}), std::move(status));
        callback();
    });
}

bool PosixEndpointImpl::Read(
    Callback on_read,
    grpc_event_engine::experimental::SliceBuffer* buffer,
    const ReadArgs* args
) {
    UASSERT(on_read && buffer);
    if (read_.in_progress.exchange(true)) {
        utils::AbortWithStacktrace("A PosixEndpoint may have only one outstanding Read, including queued callbacks");
    }
    incoming_buffer_ = buffer;
    incoming_buffer_->Clear();
    if (shutdown_.load()) {
        incoming_buffer_ = nullptr;
        engine_->Run(MakeCompletion(read_, std::move(on_read), ShutdownStatus()));
        return false;
    }
    incoming_buffer_->Swap(last_read_buffer_);
    min_progress_size_ =
        args ? static_cast<std::size_t>(std::clamp<std::int64_t>(args->read_hint_bytes, 1, kBigAlloc)) : 1;
    if (is_first_read_) {
        read_cb_ = std::move(on_read);
        is_first_read_ = false;
        NotifyOnRead();
        return false;
    }
    if (inq_ == 0) {
        read_cb_ = std::move(on_read);
        NotifyOnRead();
        return false;
    }
    y_absl::Status status;
    try {
        MaybeMakeReadSlices();
    } catch (const std::exception& error) {
        incoming_buffer_->Swap(last_read_buffer_);
        incoming_buffer_ = nullptr;
        engine_->Run(MakeCompletion(read_, std::move(on_read), y_absl::ResourceExhaustedError(error.what())));
        return false;
    }
    if (!TcpDoRead(status)) {
        read_cb_ = std::move(on_read);
        NotifyOnRead();
        return false;
    }
    incoming_buffer_ = nullptr;
    if (status.ok() && !shutdown_.load()) {
        read_.in_progress.store(false);
        return true;
    }
    engine_->Run(MakeCompletion(read_, std::move(on_read), std::move(status)));
    return false;
}

bool PosixEndpointImpl::Write(Callback on_writable, grpc_event_engine::experimental::SliceBuffer* data) {
    UASSERT(on_writable && data);
    if (write_.in_progress.exchange(true)) {
        utils::AbortWithStacktrace("A PosixEndpoint may have only one outstanding Write, including queued callbacks");
    }
    std::size_t outgoing_byte_idx = 0;
    auto result = TryIoInline([&] { return TcpFlush(*data, outgoing_byte_idx); });
    if (auto* status = std::get_if<y_absl::Status>(&result)) {
        data->Clear();
        if (status->ok() && !shutdown_.load()) {
            write_.in_progress.store(false);
            return true;
        }
        engine_->Run(MakeCompletion(write_, std::move(on_writable), std::move(*status)));
        return false;
    }
    outgoing_buffer_ = data;
    outgoing_byte_idx_ = outgoing_byte_idx;
    write_cb_ = std::move(on_writable);
    if (std::holds_alternative<WaitIo>(result)) {
        NotifyOnWrite();
    } else {
        ScheduleWrite();
    }
    return false;
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
