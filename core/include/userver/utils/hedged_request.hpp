#pragma once

/// @file userver/utils/hedged_request.hpp
/// @brief
/// Classes and functions for performing hedged requests.
///
/// To perform hedged request you need to define RequestStrategy - a class
/// similar to following ExampleStrategy:
///
/// class ExampleStrategy {
///   public:
///     /// Create request future.
///
///     /// Called at least once per hedged request. First time at the beginning
///     /// of a hedged request and then every HedgingSettings::hedging_delay
///     /// milliseconds if none of the previous requests are ready or
///     /// ProcessReply returned non-nullopt result. If ProcessReply returned
///     /// some interval of time, then additional request will be scheduled at
///     /// this interval of time.
///     /// @param attempt - increasing number for each try
///     std::optional<RequestType> Create(std::size_t attempt);
///
///     /// ProcessReply is called when some request has finished. Method should
///     /// evaluate request status and decide whether new attempt is required.
///     /// If new attempt is required, then method must return delay for next
///     /// attempt. If no other attempt is required, then method must return
///     /// std::nullopt. It is expected that successful result will be stored
///     /// somewhere internally and will be available with ExtractReply()
///     /// method.
///     std::optional<std::chrono::milliseconds> ProcessReply(RequestType&&);
///
///     std::optional<ReplyType> ExtractReply();
///
///     /// Method is called when system does not need request any more.
///     /// For example, if one of the requests has been finished successfully,
///     /// the system will finish all related subrequests.
///     /// It is recommended to make this call as fast as possible in order
///     /// not to block overall execution. For example, if you internally have
///     /// some future, it is recommended to call TryCancel() here and wait
///     /// for a cancellation in destructor, rather than call TryCancel() and
///     /// immediately wait.
///     void Finish(RequestType&&);
/// };
///
/// Then call any of these functions:
/// - HedgeRequest
/// - HedgeRequestsBulk
/// - HedgeRequestAsync
/// - HedgeRequestsBulkAsync
///

#include <chrono>
#include <functional>
#include <optional>
#include <queue>
#include <tuple>
#include <type_traits>

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::hedging {

/// Class define hedging settings
struct HedgingSettings {
  /// Maximum requests to do
  std::size_t max_attempts{3};
  /// Delay between attempts
  std::chrono::milliseconds hedging_delay{7};
  /// Max time to wait for all requests
  std::chrono::milliseconds timeout_all{100};
};

template <typename RequestStrategy>
struct RequestTraits {
  using RequestType =
      typename std::invoke_result_t<decltype(&RequestStrategy::Create),
                                    RequestStrategy, int>::value_type;
  using ReplyType =
      typename std::invoke_result_t<decltype(&RequestStrategy::ExtractReply),
                                    RequestStrategy>::value_type;
};

namespace impl {

using Clock = utils::datetime::SteadyClock;
using TimePoint = Clock::time_point;

enum class Action { StartTry, Stop };

struct PlanEntry {
 public:
  PlanEntry(TimePoint timepoint, std::size_t request_index,
            std::size_t attempt_id, Action action)
      : timepoint(timepoint),
        request_index(request_index),
        attempt_id(attempt_id),
        action(action) {}

  bool operator<(const PlanEntry& other) const noexcept {
    return tie() < other.tie();
  }
  bool operator>(const PlanEntry& other) const noexcept {
    return tie() > other.tie();
  }
  bool operator==(const PlanEntry& other) const noexcept {
    return tie() == other.tie();
  }
  bool operator<=(const PlanEntry& other) const noexcept {
    return tie() <= other.tie();
  }
  bool operator>=(const PlanEntry& other) const noexcept {
    return tie() >= other.tie();
  }
  bool operator!=(const PlanEntry& other) const noexcept {
    return tie() != other.tie();
  }

  TimePoint timepoint;
  std::size_t request_index{0};
  std::size_t attempt_id{0};
  Action action;

 private:
  std::tuple<const TimePoint&, const size_t&, const size_t&, const Action&>
  tie() const noexcept {
    return std::tie(timepoint, request_index, attempt_id, action);
  }
};

/// This wrapper allows us to cancel subrequests without removing elements
/// from vector of requests
template <typename RequestStrategy>
struct SubrequestWrapper {
  using RequestType = typename RequestTraits<RequestStrategy>::RequestType;

  SubrequestWrapper() = default;
  SubrequestWrapper(SubrequestWrapper&&) noexcept = default;
  explicit SubrequestWrapper(std::optional<RequestType>&& request)
      : request(std::move(request)) {}

  engine::impl::ContextAccessor* TryGetContextAccessor() {
    if (!request) return nullptr;
    return request->TryGetContextAccessor();
  }

  std::optional<RequestType> request;
};

struct RequestState {
  std::vector<std::size_t> subrequest_indices;
  std::size_t attempts_made = 0;
  bool finished = false;
};

template <typename RequestStrategy>
struct Context {
  using RequestType = typename RequestTraits<RequestStrategy>::RequestType;
  using ReplyType = typename RequestTraits<RequestStrategy>::ReplyType;

  Context(std::vector<RequestStrategy> inputs, HedgingSettings settings)
      : inputs_(std::move(inputs)), settings(std::move(settings)) {
    const std::size_t size = this->inputs_.size();
    request_states_.resize(size);
  }
  Context(Context&&) noexcept = default;

  void Prepare(TimePoint start_time) {
    const auto request_count = GetRequestsCount();
    for (std::size_t request_id = 0; request_id < request_count; ++request_id) {
      plan_.emplace(start_time, request_id, 0, Action::StartTry);
    }
    plan_.emplace(start_time + settings.timeout_all, 0, 0, Action::Stop);
    subrequests_.reserve(settings.max_attempts * request_count);
  }

  std::optional<TimePoint> NextEventTime() const {
    if (plan_.empty()) return std::nullopt;
    return plan_.top().timepoint;
  }
  std::optional<PlanEntry> PopPlan() {
    if (plan_.empty()) return std::nullopt;
    auto ret = plan_.top();
    plan_.pop();
    return ret;
  }
  bool IsStop() const { return stop_; }

  void FinishAllSubrequests(std::size_t request_index) {
    auto& request_state = request_states_[request_index];
    request_state.finished = true;
    const auto& subrequest_indices = request_state.subrequest_indices;
    auto& strategy = inputs_[request_index];
    for (auto i : subrequest_indices) {
      auto& request = subrequests_[i].request;
      if (request) {
        strategy.Finish(std::move(*request));
        request.reset();
      }
    }
  }

  const HedgingSettings& GetSettings() const { return settings; }

  size_t GetRequestsCount() const { return inputs_.size(); }

  size_t GetRequestIdxBySubrequestIdx(size_t subrequest_idx) const {
    return input_by_subrequests_.at(subrequest_idx);
  }

  RequestStrategy& GetStrategy(size_t index) { return inputs_[index]; }

  auto& GetSubRequests() { return subrequests_; }

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
    for (std::size_t i = 0; i < inputs_.size(); ++i) FinishAllSubrequests(i);
    stop_ = true;
  }

  /// Called on elapsed timeout of WaitAny when next event is Start retry of
  /// request with id equal @param request_index
  void OnActionStartTry(std::size_t request_index, std::size_t attempt_id,
                        TimePoint now) {
    auto& request_state = request_states_[request_index];
    if (request_state.finished) {
      return;
    }
    auto& attempts_made = request_state.attempts_made;
    // We could have already launched attempt with this number, for example if
    // attempt number 2 failed with retryable code, we will add it to plan with
    // number 3. This way, there are now two planned events, both with number=3
    // - one from retry, one from hedging. We will execute the earliest one and
    // skip the second one.
    if (attempt_id < attempts_made) {
      return;
    }

    if (attempts_made >= settings.max_attempts) {
      return;
    }
    auto& strategy = inputs_[request_index];
    auto request_opt = strategy.Create(attempts_made);
    if (!request_opt) {
      request_state.finished = true;
      // User do not want to make another request (maybe some retry budget is
      // used)
      return;
    }
    const auto idx = subrequests_.size();
    subrequests_.emplace_back(std::move(request_opt));
    request_state.subrequest_indices.push_back(idx);
    input_by_subrequests_[idx] = request_index;
    attempts_made++;
    plan_.emplace(now + settings.hedging_delay, request_index, attempts_made,
                  Action::StartTry);
  }

  /// Called on getting error in request with @param request_idx
  void OnRetriableReply(std::size_t request_idx,
                        std::chrono::milliseconds retry_delay, TimePoint now) {
    const auto& request_state = request_states_[request_idx];
    if (request_state.finished) return;
    if (request_state.attempts_made >= settings.max_attempts) return;

    plan_.emplace(now + retry_delay, request_idx, request_state.attempts_made,
                  Action::StartTry);
  }

  void OnNonRetriableReply(std::size_t request_idx) {
    FinishAllSubrequests(request_idx);
  }
  /// @}

 private:
  /// user provided request strategies bulk
  std::vector<RequestStrategy> inputs_;
  HedgingSettings settings;

  /// Our plan of what we will do at what time
  std::priority_queue<PlanEntry, std::vector<PlanEntry>, std::greater<>>
      plan_{};
  std::vector<SubrequestWrapper<RequestStrategy>> subrequests_{};
  /// Store index of input by subrequest index
  std::unordered_map<std::size_t, std::size_t> input_by_subrequests_{};
  std::vector<RequestState> request_states_{};
  bool stop_{false};
};

}  // namespace impl

/// Future of hedged bulk request
template <typename RequestStrategy>
struct HedgedRequestBulkFuture {
  using RequestType = typename RequestTraits<RequestStrategy>::RequestType;
  using ReplyType = typename RequestTraits<RequestStrategy>::ReplyType;

  HedgedRequestBulkFuture(HedgedRequestBulkFuture&&) noexcept = default;
  ~HedgedRequestBulkFuture() { task_.SyncCancel(); }

  void Wait() { task_.Wait(); }
  std::vector<std::optional<ReplyType>> Get() { return task_.Get(); }
  engine::impl::ContextAccessor* TryGetContextAccessor() {
    return task_.TryGetContextAccessor();
  }

 private:
  template <typename RequestStrategy_>
  friend auto HedgeRequestsBulkAsync(std::vector<RequestStrategy_> inputs,
                                     HedgingSettings settings);
  using Task = engine::TaskWithResult<std::vector<std::optional<ReplyType>>>;
  HedgedRequestBulkFuture(Task&& task) : task_(std::move(task)) {}
  Task task_;
};

/// Future of hedged request
template <typename RequestStrategy>
struct HedgedRequestFuture {
  using RequestType = typename RequestTraits<RequestStrategy>::RequestType;
  using ReplyType = typename RequestTraits<RequestStrategy>::ReplyType;

  HedgedRequestFuture(HedgedRequestFuture&&) noexcept = default;
  ~HedgedRequestFuture() { task_.SyncCancel(); }

  void Wait() { task_.Wait(); }
  std::optional<ReplyType> Get() { return task_.Get(); }
  void IgnoreResult() {}
  engine::impl::ContextAccessor* TryGetContextAccessor() {
    return task_.TryGetContextAccessor();
  }

 private:
  template <typename RequestStrategy_>
  friend auto HedgeRequestAsync(RequestStrategy_ input,
                                HedgingSettings settings);
  using Task = engine::TaskWithResult<std::optional<ReplyType>>;
  HedgedRequestFuture(Task&& task) : task_(std::move(task)) {}
  Task task_;
};

/// Synchronously perform bulk hedged requests described by RequestStrategy and
/// return result of type std::vector<std::optional<ResultType>>. Result
/// contains replies for each element in @param inputs or std::nullopt in case
/// either timeouts or bad replies (RequestStrategy::ProcessReply(RequestType&&)
/// returned std::nullopt and RequestStrategy::ExtractReply() returned
/// std::nullopt)
template <typename RequestStrategy>
auto HedgeRequestsBulk(std::vector<RequestStrategy> inputs,
                       HedgingSettings hedging_settings) {
  {
    using Action = impl::Action;
    using Clock = impl::Clock;
    auto context =
        impl::Context(std::move(inputs), std::move(hedging_settings));

    auto& sub_requests = context.GetSubRequests();

    auto wakeup_time = Clock::now();
    context.Prepare(wakeup_time);

    while (!context.IsStop()) {
      auto wait_result = engine::WaitAnyUntil(wakeup_time, sub_requests);
      if (!wait_result.has_value()) {
        if (engine::current_task::ShouldCancel()) {
          context.OnActionStop();
          break;
        }
        /// timeout - need to process plan
        auto plan_entry = context.PopPlan();
        if (!plan_entry.has_value()) {
          /// Timeout but we don't have planed actions any more
          break;
        }
        const auto [timestamp, request_index, attempt_id, action] = *plan_entry;
        switch (action) {
          case Action::StartTry:
            context.OnActionStartTry(request_index, attempt_id, timestamp);
            break;
          case Action::Stop:
            context.OnActionStop();
            break;
        }
        auto next_wakeup_time = context.NextEventTime();
        if (!next_wakeup_time.has_value()) {
          break;
        }
        wakeup_time = *next_wakeup_time;
        continue;
      }
      const auto result_idx = *wait_result;
      UASSERT(result_idx < sub_requests.size());
      const auto request_idx = context.GetRequestIdxBySubrequestIdx(result_idx);
      auto& strategy = context.GetStrategy(request_idx);

      auto& request = sub_requests[result_idx].request;
      UASSERT_MSG(request, "Finished requests must not be empty");
      auto reply = strategy.ProcessReply(std::move(*request));
      if (reply.has_value()) {
        /// Got reply but it's not OK and user wants to retry over
        /// some delay
        context.OnRetriableReply(request_idx, *reply, Clock::now());
        /// No need to check. we just added one entry
        wakeup_time = *context.NextEventTime();
      } else {
        context.OnNonRetriableReply(request_idx);
      }
    }
    return context.ExtractAllReplies();
  }
}

/// Asynchronously perform bulk hedged requests described by RequestStrategy and
/// return future which returns Result of type
/// std::vector<std::optional<ResultType>>. Result contains replies for each
/// element in @param inputs or std::nullopt in case either timeouts or bad
/// replies (RequestStrategy::ProcessReply(RequestType&&) returned std::nullopt
/// and RequestStrategy::ExtractReply() returned std::nulopt)
template <typename RequestStrategy>
auto HedgeRequestsBulkAsync(std::vector<RequestStrategy> inputs,
                            HedgingSettings settings) {
  return HedgedRequestBulkFuture(utils::Async(
      "hedged-bulk-request",
      [inputs{std::move(inputs)}, settings{std::move(settings)}] {
        return HedgeRequestsBulk(std::move(inputs), std::move(settings));
      }));
}

/// Synchronously Perform hedged request described by RequestStrategy and return
/// result or throw runtime_error. Exception can be thrown in case of timeout or
/// if request was denied by strategy e.g. ProcessReply always returned
/// std::nullopt or ExtractReply returned std::nullopt
template <typename RequestStrategy>
std::optional<typename RequestTraits<RequestStrategy>::ReplyType> HedgeRequest(
    RequestStrategy input, HedgingSettings settings) {
  std::vector<RequestStrategy> inputs;
  inputs.emplace_back(std::move(input));
  auto bulk_ret = HedgeRequestsBulk(std::move(inputs), std::move(settings));
  if (bulk_ret.size() != 1) {
    return std::nullopt;
  }
  return bulk_ret[0];
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
      }));
}

}  // namespace utils::hedging

USERVER_NAMESPACE_END
