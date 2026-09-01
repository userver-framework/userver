#include <userver/server/request/response_base.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

Http2StreamEventProducer::Http2StreamEventProducer(Http2StreamEventQueue& queue, engine::SingleConsumerEvent& event)
    : producer_(queue.GetProducer()),
      event_(event)
{}

void Http2StreamEventProducer::PushEvent(Http2StreamEvent event, engine::Deadline deadline) {
    const auto res = producer_.Push(std::move(event), deadline);
    UASSERT(res);
    event_.Send();
}

void Http2StreamEventProducer::CloseStream(std::int32_t id) {
    PushEvent({.stream_id = id, .body_part = "", .is_end = true});
}

}  // namespace server::http::impl

namespace server::request {

bool impl::ChunkStorage::Empty() const noexcept { return Size() == 0; }

std::size_t impl::ChunkStorage::Size() const noexcept {
    return std::visit(
        utils::Overloaded{
            [](const std::string& owned) noexcept { return owned.size(); },
            [](const std::shared_ptr<const std::string>& shared) noexcept { return shared->size(); },
        },
        storage_
    );
}

std::string_view impl::ChunkStorage::View() const noexcept { return AsString(); }

const std::string& impl::ChunkStorage::AsString() const {
    return std::visit(
        utils::Overloaded{
            [](const std::string& owned) -> const std::string& { return owned; },
            [](const std::shared_ptr<const std::string>& shared) -> const std::string& { return *shared; },
        },
        storage_
    );
}

impl::ChunkStorage::ChunkStorage(std::string data)
    : storage_{std::move(data)}
{}

impl::ChunkStorage::ChunkStorage(std::shared_ptr<const std::string> data)
    : storage_{std::move(data)}
{
    UASSERT(std::get<std::shared_ptr<const std::string>>(storage_));
}

namespace {
const auto kStartTime = std::chrono::steady_clock::now();

std::chrono::milliseconds ToMsFromStart(std::chrono::steady_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp - kStartTime);
}

// Guarantees that time_point::min() / kUnset can be used as a sentinel (before the epoch).
static_assert(std::chrono::steady_clock::duration::min() < std::chrono::steady_clock::duration::zero());
static_assert(std::chrono::steady_clock::time_point::min() < std::chrono::steady_clock::time_point{});

}  // namespace

void ResponseDataAccounter::StartRequest(std::chrono::steady_clock::time_point create_time) {
    pending_responses_count_.Add(1);
    time_sum_.Add(ToMsFromStart(create_time).count());
}

void ResponseDataAccounter::StopRequest(size_t size, std::chrono::steady_clock::time_point create_time) {
    pending_responses_size_in_bytes_ -= size;
    time_sum_.Subtract(ToMsFromStart(create_time).count());
    pending_responses_count_.Subtract(1);
}

void ResponseDataAccounter::ReaccountRequest(
    std::size_t old_size,
    std::chrono::steady_clock::time_point old_create_time,
    std::size_t new_size,
    std::chrono::steady_clock::time_point new_create_time
) {
    UASSERT(old_create_time <= new_create_time);
    pending_responses_size_in_bytes_ += new_size - old_size;
    time_sum_.Add(std::chrono::duration_cast<std::chrono::milliseconds>(new_create_time - old_create_time).count());
}

std::chrono::milliseconds ResponseDataAccounter::GetAvgRequestTime() const {
    // TODO: race
    auto count = pending_responses_count_.NonNegativeRead();
    auto time_sum = std::chrono::milliseconds(time_sum_.NonNegativeRead());

    auto now_ms = ToMsFromStart(std::chrono::steady_clock::now());
    auto delta = (now_ms * count) - time_sum;
    return delta / (count ? count : 1);
}

ResponseBase::ResponseBase(ResponseDataAccounter& data_accounter)
    : ResponseBase{data_accounter, std::chrono::steady_clock::now()}
{}

ResponseBase::ResponseBase(ResponseDataAccounter& data_account, std::chrono::steady_clock::time_point now)
    : accounter_{data_account},
      create_time_{now}
{
    UASSERT(accounted_size_ == 0);
    UASSERT(data_.Empty());
    accounter_.StartRequest(create_time_);
}

ResponseBase::~ResponseBase() noexcept {
    if (!IsSent()) {
        accounter_.StopRequest(accounted_size_, create_time_);
    }
}

void ResponseBase::SetData(std::string data) { StoreData(impl::ChunkStorage{std::move(data)}); }

void ResponseBase::SetSharedData(std::shared_ptr<const std::string> data) {
    UASSERT(data);
    StoreData(impl::ChunkStorage{std::move(data)});
}

void ResponseBase::StoreData(impl::ChunkStorage data) {
    if (IsSent()) {
        UASSERT(IsBodyStreamed());
        LOG_LIMITED_WARNING()
            << "Attempt to set response body after it was already sent by streaming. Probably an "
               "exception was thrown after streaming started";
        return;
    }
    data_ = std::move(data);
    const auto old_size = accounted_size_;
    const auto old_create_time = create_time_;
    create_time_ = std::chrono::steady_clock::now();
    accounted_size_ = data_.Size();
    accounter_.ReaccountRequest(old_size, old_create_time, accounted_size_, create_time_);
}

const std::string& ResponseBase::GetData() const { return data_.AsString(); }

impl::ChunkStorage ResponseBase::ExtractData() { return std::move(data_); }

void ResponseBase::SetReady() { SetReady(std::chrono::steady_clock::now()); }

void ResponseBase::SetReady(std::chrono::steady_clock::time_point now) {
    UASSERT(now != kUnset);
    ready_time_ = now;
}

bool ResponseBase::IsLimitReached() const {
    return accounter_.GetPendingResponsesSizeInBytes() >= accounter_.GetMaxPendingResponsesSizeInBytes();
}

void ResponseBase::SetSendFailed() { SetSent(0); }

void ResponseBase::SetSent(std::size_t bytes_sent) {
    UASSERT(!IsSent());
    accounter_.StopRequest(accounted_size_, create_time_);
    bytes_sent_ = bytes_sent;
    is_sent_ = true;
}

void ResponseBase::SetStreamId(std::int32_t stream_id) {
    UASSERT(!stream_id_.has_value());
    stream_id_.emplace(stream_id);
}

void ResponseBase::SetStreamProdicer(http::impl::Http2StreamEventProducer&& producer) {
    UASSERT(!producer_.has_value());
    producer_.emplace(std::move(producer));
}

http::impl::Http2StreamEventProducer ResponseBase::GetStreamProducer() {
    UASSERT(producer_);
    return std::move(producer_.value());
}

}  // namespace server::request

USERVER_NAMESPACE_END
