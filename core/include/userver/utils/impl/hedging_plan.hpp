#pragma once

#include <chrono>
#include <compare>
#include <cstddef>
#include <optional>
#include <vector>

#include <userver/utils/datetime.hpp>
#include <userver/utils/hedging_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::hedging::impl {

using Clock = utils::datetime::SteadyClock;
using TimePoint = Clock::time_point;

enum class Action { kStartTry, kStop };

struct PlanEntry {
    PlanEntry(TimePoint timepoint, std::size_t request_index, std::size_t attempt_id, Action action)
        : timepoint(timepoint),
          request_index(request_index),
          attempt_id(attempt_id),
          action(action)
    {}

    auto operator<=>(const PlanEntry& other) const noexcept = default;

    TimePoint timepoint;
    std::size_t request_index{0};
    std::size_t attempt_id{0};
    Action action;
};

struct RequestState {
    std::vector<std::size_t> subrequest_indices;
    std::size_t attempts_made = 0;
    bool finished = false;
    TimePoint not_before{};
};

/// Plan of how N user requests (inputs) are executed. Because of retries and hedging, these N requests turn into a
/// larger number of actual requests. All actual requests share a single numbering: their index in
/// Context::subrequests_. The plan maintains two indices:
/// 1. From the index K of a user request to all of its attempts: request_states_[K].subrequest_indices lists the
///    indices to look up in subrequests_.
/// 2. The reverse one, from an index in subrequests_ to the user request it was made for: input_by_subrequests_.
///
/// The first index allows, for example, cancelling all the remaining attempts of request K once one of its attempts
/// gets a final reply. The second one allows finding the user request, and thus its strategy, for an attempt that
/// WaitAnyContext reported as finished.
class HedgingPlan final {
public:
    HedgingPlan(std::size_t request_count, HedgingSettings settings, TimePoint start_time);

    std::size_t GetMaxSubrequestsCount() const;

    std::optional<TimePoint> NextEventTime() const;
    std::optional<PlanEntry> PopPlan();

    bool IsStop() const { return stop_; }
    void Stop() { stop_ = true; }

    bool HasStartableRequests() const { return startable_requests_count_ != 0; }

    std::size_t GetRequestIdxBySubrequestIdx(std::size_t subrequest_idx) const;

    const std::vector<std::size_t>& GetSubrequestIndices(std::size_t request_index) const;

    bool ShouldStartTry(std::size_t request_index, std::size_t attempt_id, TimePoint now) const;

    void OnTryStarted(std::size_t request_index, std::size_t subrequest_idx, TimePoint now);

    void OnRetriableReply(std::size_t request_idx, std::chrono::milliseconds retry_delay, TimePoint now);

    void FinishRequest(std::size_t request_index);

private:
    bool CanStartAttempts(const RequestState& request_state) const;

    void AddPlanEntry(PlanEntry entry);

    HedgingSettings settings_;
    TimePoint stop_time_;

    std::vector<PlanEntry> plan_heap_{};
    std::vector<RequestState> request_states_;
    /// Store index of input by subrequest index
    std::vector<std::size_t> input_by_subrequests_{};
    std::size_t startable_requests_count_{0};
    bool stop_{false};
};

/// Returns the current time of the plan: the latest of the clock time and the time of the last processed plan event.
/// The mocked time in testsuite may be frozen, so the plan time has to advance with the processed plan events,
/// otherwise delays are measured from the same point over and over and `timeout_all` is never reached.
TimePoint GetPlanNow(TimePoint last_event_time);

void InterruptibleSleepUntilIfNotReached(TimePoint wakeup_time, TimePoint now);

}  // namespace utils::hedging::impl

USERVER_NAMESPACE_END
