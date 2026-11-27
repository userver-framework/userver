#pragma once

/// @file userver/utils/hedged_request.hpp
/// @brief Classes and functions for performing hedged requests.
///
/// To perform hedged request you need to define RequestStrategy - a class
/// similar to the following ExampleStrategy:
///
/// @snippet core/src/utils/hedged_request_test.cpp HedgedRequest ExampleStrategy
///
/// Then call any of these functions:
/// - HedgeRequest
/// - HedgeRequestsBulk
/// - HedgeRequestAsync
/// - HedgeRequestsBulkAsync
///

#include <chrono>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/hedging_settings.hpp>
#include <userver/utils/impl/hedging_plan.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::hedging {

template <typename RequestStrategy>
struct RequestTraits {
    using RequestType =
        typename std::invoke_result_t<decltype(&RequestStrategy::Create), RequestStrategy, int>::value_type;
    using ReplyType =
        typename std::invoke_result_t<decltype(&RequestStrategy::ExtractReply), RequestStrategy>::value_type;
};

namespace impl {

/// This wrapper allows us to cancel subrequests without removing elements
/// from vector of requests
template <typename RequestStrategy>
struct SubrequestWrapper {
    using RequestType = typename RequestTraits<RequestStrategy>::RequestType;

    SubrequestWrapper() = default;
    SubrequestWrapper(SubrequestWrapper&&) noexcept = default;
    explicit SubrequestWrapper(std::optional<RequestType>&& request)
        : request(std::move(request))
    {}

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    engine::AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND {
        if (!request) {
            return engine::AwaitableToken{};
        }
        return request->GetAwaitableToken();
    }

    std::optional<RequestType> request;
    engine::WaitAnyContext::Handle wait_any_handle;
};

template <typename RequestStrategy>
struct Context {
    using RequestType = typename RequestTraits<RequestStrategy>::RequestType;
    using ReplyType = typename RequestTraits<RequestStrategy>::ReplyType;

    Context(std::vector<RequestStrategy> inputs, HedgingSettings settings, TimePoint start_time)
        : inputs_(std::move(inputs)),
          plan_(inputs_.size(), std::move(settings), start_time)
    {
        subrequests_.reserve(plan_.GetMaxSubrequestsCount());
    }
    Context(Context&&) noexcept = default;

    HedgingPlan& GetPlan() { return plan_; }

    void FinishAllSubrequests(std::size_t request_index) {
        plan_.FinishRequest(request_index);
        auto& strategy = inputs_[request_index];
        for (auto i : plan_.GetSubrequestIndices(request_index)) {
            auto& subrequest = subrequests_[i];
            if (subrequest.request) {
                // The request is going to be destroyed, so it must not be waited for
                wait_any_.Remove(subrequest.wait_any_handle);
                strategy.Finish(std::move(*subrequest.request));
                subrequest.request.reset();
            }
        }
    }

    size_t GetRequestsCount() const { return inputs_.size(); }

    RequestStrategy& GetStrategy(size_t index) { return inputs_[index]; }

    auto& GetSubRequests() { return subrequests_; }

    engine::WaitAnyContext& GetWaitAny() { return wait_any_; }

    std::vector<std::optional<ReplyType>> ExtractAllReplies() {
        std::vector<std::optional<ReplyType>> ret;
        ret.reserve(GetRequestsCount());
        for (auto&& strategy : inputs_) {
            ret.emplace_back(strategy.ExtractReply());
        }
        return ret;
    }

    /// @name Reactions on events with WaitAny*
    /// @{
    /// Called on elapsed timeout of WaitAny when next event is Stop some
    /// request
    void OnActionStop() {
        for (std::size_t i = 0; i < inputs_.size(); ++i) {
            FinishAllSubrequests(i);
        }
        plan_.Stop();
    }

    /// Called on elapsed timeout of WaitAny when next event is Start retry of
    /// request with id equal @param request_index
    void OnActionStartTry(std::size_t request_index, std::size_t attempt_id, TimePoint now) {
        if (!plan_.ShouldStartTry(request_index, attempt_id, now)) {
            return;
        }
        auto& strategy = inputs_[request_index];
        auto request_opt = strategy.Create(attempt_id);
        if (!request_opt) {
            plan_.FinishRequest(request_index);
            // User do not want to make another request (maybe some retry budget is
            // used)
            return;
        }
        UASSERT_MSG(
            subrequests_.size() < subrequests_.capacity(),
            "Subrequests are waited by wait_any_ and must not be reallocated"
        );
        const auto idx = subrequests_.size();
        auto& subrequest = subrequests_.emplace_back(std::move(request_opt));
        UASSERT_MSG(!subrequest.GetAwaitableToken().IsEmpty(), "The created request must be awaitable");
        subrequest.wait_any_handle = wait_any_.Append(idx, subrequest);
        plan_.OnTryStarted(request_index, idx, now);
    }

    void OnNonRetriableReply(std::size_t request_idx) { FinishAllSubrequests(request_idx); }
    /// @}

private:
    /// user provided request strategies bulk
    std::vector<RequestStrategy> inputs_;
    HedgingPlan plan_;
    std::vector<SubrequestWrapper<RequestStrategy>> subrequests_{};
    /// Waits for subrequests_ in flight, must be destroyed before them
    engine::WaitAnyContext wait_any_{};
};

}  // namespace impl

/// Future of hedged bulk request
template <typename RequestStrategy>
struct HedgedRequestBulkFuture {
    using RequestType = typename RequestTraits<RequestStrategy>::RequestType;
    using ReplyType = typename RequestTraits<RequestStrategy>::ReplyType;

    HedgedRequestBulkFuture(HedgedRequestBulkFuture&&) noexcept = default;
    ~HedgedRequestBulkFuture() { task_.SyncCancel(); }

    /// @brief Wait for the request finish or for a caller task cancellation.
    void Wait() { task_.Wait(); }

    /// @copydoc engine::TaskWithResult::Get()
    std::vector<std::optional<ReplyType>> Get() { return task_.Get(); }

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    engine::AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND { return task_.GetAwaitableToken(); }

private:
    template <typename TRequestStrategy>
    friend auto HedgeRequestsBulkAsync(std::vector<TRequestStrategy> inputs, HedgingSettings settings);
    using Task = engine::TaskWithResult<std::vector<std::optional<ReplyType>>>;
    HedgedRequestBulkFuture(Task&& task)
        : task_(std::move(task))
    {}
    Task task_;
};

/// Future of hedged request
template <typename RequestStrategy>
struct HedgedRequestFuture {
    using RequestType = typename RequestTraits<RequestStrategy>::RequestType;
    using ReplyType = typename RequestTraits<RequestStrategy>::ReplyType;

    HedgedRequestFuture(HedgedRequestFuture&&) noexcept = default;
    ~HedgedRequestFuture() { task_.SyncCancel(); }

    /// @brief Wait for the request finish or for a caller task cancellation.
    void Wait() { task_.Wait(); }

    /// @copydoc engine::TaskWithResult::Get()
    std::optional<ReplyType> Get() { return task_.Get(); }

    void IgnoreResult() {}

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    engine::AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND { return task_.GetAwaitableToken(); }

private:
    template <typename TRequestStrategy>
    friend auto HedgeRequestAsync(TRequestStrategy input, HedgingSettings settings);
    using Task = engine::TaskWithResult<std::optional<ReplyType>>;
    HedgedRequestFuture(Task&& task)
        : task_(std::move(task))
    {}
    Task task_;
};

/// @brief Synchronously perform bulk hedged requests described by `RequestStrategy` and
/// return result of type `std::vector<std::optional<ResultType>>`.
///
/// Result contains replies for each element in `inputs` or `std::nullopt` in case
/// of either timeouts or bad replies (`RequestStrategy::ProcessReply(RequestType&&)`
/// returned `std::nullopt` and `RequestStrategy::ExtractReply()` returned
/// `std::nullopt`).
template <typename RequestStrategy>
auto HedgeRequestsBulk(std::vector<RequestStrategy> inputs, HedgingSettings hedging_settings) {
    {
        using Action = impl::Action;
        using Clock = impl::Clock;
        auto wakeup_time = Clock::now();
        // The mocked time in testsuite may be frozen, so "now" of the plan is taken via GetPlanNow from the time of
        // the last processed plan event too
        auto last_event_time = wakeup_time;
        auto context = impl::Context(std::move(inputs), std::move(hedging_settings), wakeup_time);

        auto& plan = context.GetPlan();
        auto& sub_requests = context.GetSubRequests();
        auto& wait_any = context.GetWaitAny();

        while (!plan.IsStop()) {
            const auto now = impl::GetPlanNow(last_event_time);
            const auto wait_result = wait_any.WaitFor(wakeup_time - now);
            if (!wait_result.has_value()) {
                if (wait_result.error() == engine::WaitAnyError::kEmpty) {
                    if (!plan.HasStartableRequests()) {
                        context.OnActionStop();
                        break;
                    }
                    impl::InterruptibleSleepUntilIfNotReached(wakeup_time, now);
                }
                if (engine::current_task::ShouldCancel()) {
                    context.OnActionStop();
                    break;
                }
                /// timeout - need to process plan
                auto plan_entry = plan.PopPlan();
                if (!plan_entry.has_value()) {
                    /// Timeout but we don't have planned actions any more
                    break;
                }
                const auto [timestamp, request_index, attempt_id, action] = *plan_entry;
                last_event_time = timestamp;
                switch (action) {
                    case Action::kStartTry:
                        context.OnActionStartTry(request_index, attempt_id, timestamp);
                        break;
                    case Action::kStop:
                        context.OnActionStop();
                        break;
                }
                auto next_wakeup_time = plan.NextEventTime();
                if (!next_wakeup_time.has_value()) {
                    break;
                }
                wakeup_time = *next_wakeup_time;
                continue;
            }
            const auto result_idx = *wait_result;
            UASSERT(result_idx < sub_requests.size());
            const auto request_idx = plan.GetRequestIdxBySubrequestIdx(result_idx);
            auto& strategy = context.GetStrategy(request_idx);

            sub_requests[result_idx].wait_any_handle = {};
            auto& request = sub_requests[result_idx].request;
            UASSERT_MSG(request, "Finished requests must not be empty");
            auto reply = strategy.ProcessReply(std::move(*request));
            if (reply.has_value()) {
                /// Got reply but it's not OK and user wants to retry over
                /// some delay
                plan.OnRetriableReply(request_idx, *reply, impl::GetPlanNow(last_event_time));
                /// No need to check, the stop entry is planned until the stop
                wakeup_time = *plan.NextEventTime();
            } else {
                context.OnNonRetriableReply(request_idx);
            }
        }
        return context.ExtractAllReplies();
    }
}

/// @brief Asynchronously perform bulk hedged requests described by `RequestStrategy` and
/// return future which returns Result of type `std::vector<std::optional<ResultType>>`.
///
/// Result contains replies for each
/// element in `inputs` or `std::nullopt` in case of either timeouts or bad
/// replies (`RequestStrategy::ProcessReply(RequestType&&)` returned `std::nullopt`
/// and `RequestStrategy::ExtractReply()` returned `std::nullopt`).
template <typename RequestStrategy>
auto HedgeRequestsBulkAsync(std::vector<RequestStrategy> inputs, HedgingSettings settings) {
    return HedgedRequestBulkFuture<RequestStrategy>(utils::Async(
        "hedged-bulk-request",
        [inputs{std::move(inputs)}, settings{std::move(settings)}]() mutable {
            return HedgeRequestsBulk(std::move(inputs), std::move(settings));
        }
    ));
}

/// Synchronously Perform hedged request described by RequestStrategy and return
/// result or throw runtime_error. Exception can be thrown in case of timeout or
/// if request was denied by strategy e.g. ProcessReply always returned
/// std::nullopt or ExtractReply returned std::nullopt
template <typename RequestStrategy>
std::optional<typename RequestTraits<RequestStrategy>::ReplyType> HedgeRequest(
    RequestStrategy input,
    HedgingSettings settings
) {
    std::vector<RequestStrategy> inputs;
    inputs.emplace_back(std::move(input));
    auto bulk_ret = HedgeRequestsBulk(std::move(inputs), std::move(settings));
    if (bulk_ret.size() != 1) {
        return std::nullopt;
    }
    return std::move(bulk_ret[0]);
}

/// Create future which perform hedged request described by RequestStrategy and
/// return result or throw runtime_error. Exception can be thrown in case of
/// timeout or if request was denied by strategy e.g. ProcessReply always
/// returned std::nullopt or ExtractReply returned std::nullopt
template <typename RequestStrategy>
auto HedgeRequestAsync(RequestStrategy input, HedgingSettings settings) {
    return HedgedRequestFuture<RequestStrategy>(utils::Async(
        "hedged-request",
        [input{std::move(input)}, settings{std::move(settings)}]() mutable {
            return HedgeRequest(std::move(input), std::move(settings));
        }
    ));
}

}  // namespace utils::hedging

USERVER_NAMESPACE_END
