#include <server/request/response_data_accounter.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {

const auto kStartTime = std::chrono::steady_clock::now();

std::chrono::milliseconds ToMsFromStart(std::chrono::steady_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp - kStartTime);
}

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

}  // namespace server::request

USERVER_NAMESPACE_END
