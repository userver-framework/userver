#include <userver/utils/impl/hedging_plan.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <utility>

#include <userver/engine/sleep.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::hedging::impl {

HedgingPlan::HedgingPlan(std::size_t request_count, HedgingSettings settings, TimePoint start_time)
    : settings_(std::move(settings)),
      stop_time_(start_time + settings_.timeout_all),
      request_states_(request_count)
{
    if (settings_.max_attempts != 0) {
        startable_requests_count_ = request_count;
    }
    plan_heap_.reserve(request_count + 1);
    for (std::size_t request_id = 0; request_id < request_count; ++request_id) {
        AddPlanEntry(PlanEntry{start_time, request_id, 0, Action::kStartTry});
    }
    AddPlanEntry(PlanEntry{stop_time_, 0, 0, Action::kStop});
    input_by_subrequests_.reserve(GetMaxSubrequestsCount());
}

std::size_t HedgingPlan::GetMaxSubrequestsCount() const { return settings_.max_attempts * request_states_.size(); }

std::optional<TimePoint> HedgingPlan::NextEventTime() const {
    if (plan_heap_.empty()) {
        return std::nullopt;
    }
    return plan_heap_.front().timepoint;
}

std::optional<PlanEntry> HedgingPlan::PopPlan() {
    if (plan_heap_.empty()) {
        return std::nullopt;
    }
    std::pop_heap(plan_heap_.begin(), plan_heap_.end(), std::greater<>{});
    const auto ret = plan_heap_.back();
    plan_heap_.pop_back();
    return ret;
}

std::size_t HedgingPlan::GetRequestIdxBySubrequestIdx(std::size_t subrequest_idx) const {
    UASSERT(subrequest_idx < input_by_subrequests_.size());
    return input_by_subrequests_[subrequest_idx];
}

const std::vector<std::size_t>& HedgingPlan::GetSubrequestIndices(std::size_t request_index) const {
    return request_states_[request_index].subrequest_indices;
}

bool HedgingPlan::ShouldStartTry(std::size_t request_index, std::size_t attempt_id, TimePoint now) const {
    const auto& request_state = request_states_[request_index];
    UASSERT(attempt_id <= request_state.attempts_made);
    // We could have already launched attempt with this number, for example if
    // attempt number 2 failed with retryable code, we will add it to plan with
    // number 3. This way, there are now two planned events, both with number=3
    // - one from retry, one from hedging. We will execute the earliest one and
    // skip the second one.
    const bool is_already_started = attempt_id < request_state.attempts_made;
    return CanStartAttempts(request_state) && !is_already_started && now >= request_state.not_before;
}

void HedgingPlan::OnTryStarted(std::size_t request_index, std::size_t subrequest_idx, TimePoint now) {
    auto& request_state = request_states_[request_index];
    UASSERT(CanStartAttempts(request_state));
    request_state.subrequest_indices.push_back(subrequest_idx);
    UASSERT(subrequest_idx == input_by_subrequests_.size());
    input_by_subrequests_.push_back(request_index);
    ++request_state.attempts_made;
    if (!CanStartAttempts(request_state)) {
        --startable_requests_count_;
    }
    AddPlanEntry(PlanEntry{
        now + settings_.hedging_delay,
        request_index,
        request_state.attempts_made,
        Action::kStartTry,
    });
}

void HedgingPlan::OnRetriableReply(std::size_t request_idx, std::chrono::milliseconds retry_delay, TimePoint now) {
    auto& request_state = request_states_[request_idx];
    if (!CanStartAttempts(request_state)) {
        return;
    }

    const auto time_left = std::chrono::ceil<std::chrono::milliseconds>(stop_time_ - now);
    if (retry_delay >= time_left) {
        FinishRequest(request_idx);
        return;
    }
    const auto start_time = std::max(request_state.not_before, now + retry_delay);
    request_state.not_before = start_time;
    AddPlanEntry(PlanEntry{start_time, request_idx, request_state.attempts_made, Action::kStartTry});
}

void HedgingPlan::FinishRequest(std::size_t request_index) {
    auto& request_state = request_states_[request_index];
    if (CanStartAttempts(request_state)) {
        --startable_requests_count_;
    }
    request_state.finished = true;
}

bool HedgingPlan::CanStartAttempts(const RequestState& request_state) const {
    return !request_state.finished && request_state.attempts_made < settings_.max_attempts;
}

void HedgingPlan::AddPlanEntry(PlanEntry entry) {
    plan_heap_.push_back(entry);
    std::push_heap(plan_heap_.begin(), plan_heap_.end(), std::greater<>{});
}

TimePoint GetPlanNow(TimePoint last_event_time) { return std::max(Clock::now(), last_event_time); }

void InterruptibleSleepUntilIfNotReached(TimePoint wakeup_time, TimePoint now) {
    const auto time_left = wakeup_time - now;
    if (time_left > TimePoint::duration::zero()) {
        engine::InterruptibleSleepFor(time_left);
    }
}

}  // namespace utils::hedging::impl

USERVER_NAMESPACE_END
