#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <limits>

#include <userver/concurrent/striped_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class ResponseDataAccounter final {
public:
    void StartRequest(std::chrono::steady_clock::time_point create_time);

    void StopRequest(std::size_t size, std::chrono::steady_clock::time_point create_time);

    void ReaccountRequest(
        std::size_t old_size,
        std::chrono::steady_clock::time_point old_create_time,
        std::size_t new_size,
        std::chrono::steady_clock::time_point new_create_time
    );

    std::size_t GetPendingResponsesSizeInBytes() const { return pending_responses_size_in_bytes_; }

    std::size_t GetPendingResponsesCount() const { return pending_responses_count_.NonNegativeRead(); }

    std::size_t GetMaxPendingResponsesSizeInBytes() const { return max_pending_responses_size_in_bytes_; }

    void SetMaxPendingResponsesSizeInBytes(size_t size) { max_pending_responses_size_in_bytes_ = size; }

    std::chrono::milliseconds GetAvgRequestTime() const;

private:
    std::atomic<std::size_t> pending_responses_size_in_bytes_{0};
    std::atomic<std::size_t> max_pending_responses_size_in_bytes_{std::numeric_limits<std::size_t>::max()};
    concurrent::StripedCounter pending_responses_count_{};
    concurrent::StripedCounter time_sum_{};
};

}  // namespace server::request

USERVER_NAMESPACE_END
