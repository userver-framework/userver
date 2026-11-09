#include <userver/utils/hedged_request.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <userver/engine/future.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/mock_now.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kDefaultDelay = 100ms;

using SteadyTimePoint = std::chrono::steady_clock::time_point;

/// [HedgedRequest ExampleStrategy]
struct ExampleResponse {
    std::string body;
    // The server asks to retry not earlier than after this delay, e.g. via `grpc-retry-pushback-ms`
    std::optional<std::chrono::milliseconds> retry_after;
};

class ExampleStrategy {
public:
    explicit ExampleStrategy(std::size_t failed_attempts)
        : failed_attempts_(failed_attempts)
    {}

    // Creates the request of an attempt. Called at the beginning of the hedged request, then every
    // HedgingSettings::hedging_delay while there is no accepted reply, and after the delay returned by
    // ProcessReply. Returning std::nullopt stops the attempts of the request. The awaitable token of the
    // returned request must not be empty and must not change until the request is finished.
    std::optional<engine::TaskWithResult<ExampleResponse>> Create(std::size_t attempt) {
        const bool is_failed = attempt < failed_attempts_;
        return utils::Async("example-request", [is_failed, attempt] {
            if (is_failed) {
                return ExampleResponse{.body = {}, .retry_after = 10ms};
            }
            return ExampleResponse{.body = "attempt " + std::to_string(attempt), .retry_after = std::nullopt};
        });
    }

    // Called when some attempt has finished. Returns std::nullopt if no other attempt is required, storing
    // the successful result to be returned by ExtractReply. Otherwise returns the delay of the next attempt:
    // no attempts of the request, hedging ones included, are started earlier than after the delay. If the
    // delay ends after HedgingSettings::timeout_all since the start of the hedged request, no more attempts
    // are started, while the attempts in flight are still waited for.
    std::optional<std::chrono::milliseconds> ProcessReply(engine::TaskWithResult<ExampleResponse>&& request) {
        auto response = request.Get();
        if (response.retry_after.has_value()) {
            return response.retry_after;
        }
        reply_ = std::move(response.body);
        return std::nullopt;
    }

    std::optional<std::string> ExtractReply() { return std::move(reply_); }

    // Called when the request is not needed any more, e.g. another attempt has succeeded. Is also called
    // for the requests already passed to ProcessReply. Should be fast: it is recommended to request
    // cancellation here and to wait for it in the destructor of the request.
    void Finish(engine::TaskWithResult<ExampleResponse>&& request) {
        if (request.IsValid()) {
            request.RequestCancel();
        }
    }

private:
    std::size_t failed_attempts_;
    std::optional<std::string> reply_;
};
/// [HedgedRequest ExampleStrategy]

struct TestFuture {
    TestFuture(std::string reply, size_t attempt, std::chrono::milliseconds delay)
        : task(utils::Async(
              "test",
              [reply, delay] {
                  engine::SleepFor(delay);
                  return reply;
              }
          )),
          attempt(attempt)
    {}
    TestFuture(TestFuture&&) noexcept = default;
    std::string Get() { return task.Get(); }
    engine::AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND { return task.GetAwaitableToken(); }

    engine::TaskWithResult<std::string> task;
    size_t attempt{0};
};

enum class Event { kStartRequest, kProcessReply, kFinish };
std::string_view ToString(Event event) {
    switch (event) {
        case Event::kStartRequest:
            return "StartRequest";
        case Event::kProcessReply:
            return "ProcessReply";
        case Event::kFinish:
            return "Finish";
    }
    return "Invalid";
}
struct EventLogEntry {
    bool operator==(const EventLogEntry& other) const noexcept {
        return std::tie(event, attempt) == std::tie(other.event, other.attempt);
    }

    Event event;
    size_t attempt;
    SteadyTimePoint time{};
};
std::ostream& operator<<(std::ostream& str, const EventLogEntry& event) {
    str << "{" << ToString(event.event) << ", " << event.attempt << "}";
    return str;
}
using EventLog = std::vector<EventLogEntry>;

struct AttemptProgramEntry {
    std::chrono::milliseconds delay{kDefaultDelay};
    std::optional<std::chrono::milliseconds> process_reply_result;
};
using AttemptProgram = std::vector<AttemptProgramEntry>;

class TestStrategy {
public:
    using RequestType = TestFuture;
    using ReplyType = std::string;

    TestStrategy(EventLog& event_log, AttemptProgram attempt_program)
        : attempt_program(std::move(attempt_program)),
          event_log(event_log)
    {}

    std::optional<RequestType> Create(size_t attempt) {
        event_log
            .push_back({.event = Event::kStartRequest, .attempt = attempt, .time = std::chrono::steady_clock::now()});
        auto delay = kDefaultDelay;
        if (attempt_program.size() > attempt) {
            delay = attempt_program[attempt].delay;
        }
        return TestFuture("Request" + std::to_string(attempt), attempt, delay);
    }

    std::optional<std::chrono::milliseconds> ProcessReply(RequestType&& request) {
        const size_t attempt = request.attempt;
        reply = std::move(request).Get();
        event_log
            .push_back({.event = Event::kProcessReply, .attempt = attempt, .time = std::chrono::steady_clock::now()});
        if (attempt_program.size() <= attempt) {
            return std::nullopt;
        }
        return attempt_program[attempt].process_reply_result;
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    std::optional<ReplyType> ExtractReply() { return reply; }

    void Finish(RequestType&& request) {
        event_log.push_back({.event = Event::kFinish, .attempt = request.attempt});
        if (request.task.IsValid()) {
            request.task.SyncCancel();
        }
    }

    std::optional<ReplyType> reply;
    AttemptProgram attempt_program;
    EventLog& event_log;
};

class MoveOnlyReplyStrategy : public TestStrategy {
public:
    using TestStrategy::TestStrategy;
    using ReplyType = std::unique_ptr<std::string>;

    std::optional<ReplyType> ExtractReply() {
        if (!reply) {
            return std::nullopt;
        }
        return std::make_unique<std::string>(std::move(*reply));
    }
};

std::chrono::milliseconds GetDuration(const utils::hedging::HedgingSettings& settings, TestStrategy strategy) {
    const auto start = std::chrono::steady_clock::now();
    utils::hedging::HedgeRequest(std::move(strategy), settings);
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
}

std::optional<SteadyTimePoint> FindEventTime(const EventLog& event_log, Event event, size_t attempt) {
    const auto it = std::find(event_log.begin(), event_log.end(), EventLogEntry{.event = event, .attempt = attempt});
    if (it == event_log.end()) {
        return std::nullopt;
    }
    return it->time;
}

using ReplyResult = std::optional<std::chrono::milliseconds>;

struct ControlledRequest {
    engine::AwaitableToken GetAwaitableToken() USERVER_IMPL_LIFETIME_BOUND { return future.GetAwaitableToken(); }

    engine::Future<ReplyResult> future;
    size_t attempt{0};
};

/// Attempts of ControlledStrategy, the results of ProcessReply are set by the test
struct ControlledAttempts {
    [[nodiscard]] bool WaitForCreated(size_t count) {
        while (promises.size() < count) {
            if (!created.WaitForEventFor(utest::kMaxTestWaitTime)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool WaitForReplied(size_t count) {
        while (reply_times.size() < count) {
            if (!replied.WaitForEventFor(utest::kMaxTestWaitTime)) {
                return false;
            }
        }
        return true;
    }

    size_t attempts_limit{std::numeric_limits<size_t>::max()};
    size_t create_calls{0};
    size_t process_reply_calls{0};
    size_t finish_calls{0};
    std::vector<engine::Promise<ReplyResult>> promises;
    std::vector<SteadyTimePoint> create_times;
    std::map<size_t, SteadyTimePoint> reply_times;
    engine::SingleConsumerEvent created;
    engine::SingleConsumerEvent replied;
};

class ControlledStrategy {
public:
    explicit ControlledStrategy(ControlledAttempts& attempts)
        : attempts_(attempts)
    {}

    std::optional<ControlledRequest> Create(size_t attempt) {
        ++attempts_.create_calls;
        if (attempt >= attempts_.attempts_limit) {
            return std::nullopt;
        }
        auto& promise = attempts_.promises.emplace_back();
        attempts_.create_times.push_back(std::chrono::steady_clock::now());
        attempts_.created.Send();
        return ControlledRequest{.future = promise.get_future(), .attempt = attempt};
    }

    ReplyResult ProcessReply(ControlledRequest&& request) {
        ++attempts_.process_reply_calls;
        attempts_.reply_times.emplace(request.attempt, std::chrono::steady_clock::now());
        attempts_.replied.Send();
        auto result = request.future.get();
        if (!result.has_value()) {
            reply_ = "Request" + std::to_string(request.attempt);
        }
        return result;
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    std::optional<std::string> ExtractReply() { return reply_; }

    void Finish(ControlledRequest&&) { ++attempts_.finish_calls; }

private:
    ControlledAttempts& attempts_;
    std::optional<std::string> reply_;
};

}  // namespace

UTEST(HedgedRequest, MoveOnlyReply) {
    const utils::hedging::HedgingSettings settings{1, 0ms, 1000ms};
    EventLog event_log;
    auto ret = utils::hedging::HedgeRequest(MoveOnlyReplyStrategy(event_log, {}), settings);
    ASSERT_TRUE(ret.has_value());
    ASSERT_NE(*ret, nullptr);
    EXPECT_EQ(**ret, "Request0");
}

UTEST(HedgedRequest, MoveOnlyReplyAsync) {
    const utils::hedging::HedgingSettings settings{1, 0ms, 1000ms};
    EventLog event_log;
    auto future = utils::hedging::HedgeRequestAsync(MoveOnlyReplyStrategy(event_log, {}), settings);
    auto ret = future.Get();
    ASSERT_TRUE(ret.has_value());
    ASSERT_NE(*ret, nullptr);
    EXPECT_EQ(**ret, "Request0");
}

UTEST(HedgedRequest, Base) {
    /// Test basic case:
    /// make 3 consecutive requests with 10 ms delay. Get reply from first attempt
    /// and cancel all requests
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kStartRequest, .attempt = 1},
        {.event = Event::kStartRequest, .attempt = 2},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kFinish, .attempt = 0},
        {.event = Event::kFinish, .attempt = 1},
        {.event = Event::kFinish, .attempt = 2},
    };
    const utils::hedging::HedgingSettings settings{3, 10ms, 1000ms};
    EventLog event_log;
    engine::Yield();
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, {}), settings);
    EXPECT_EQ(ret, "Request0");
    EXPECT_EQ(event_log, expected_event_log);
}

UTEST(HedgedRequest, DontStartIfGotResult) {
    /// Test we do not start additional requests if we got reply before
    /// hedging_delay elapsed
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kFinish, .attempt = 0},
    };
    const utils::hedging::HedgingSettings settings{3, 200ms, 1000ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = std::nullopt},
    };

    EventLog event_log;
    engine::Yield();
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);
    EXPECT_EQ(ret, "Request0");
    EXPECT_EQ(event_log, expected_event_log);
}

UTEST(HedgedRequest, SuccessfulHedging) {
    /// Test case when second attempt finish first
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kStartRequest, .attempt = 1},
        {.event = Event::kProcessReply, .attempt = 1},
        {.event = Event::kFinish, .attempt = 0},
        {.event = Event::kFinish, .attempt = 1},
    };
    const utils::hedging::HedgingSettings settings{2, 10ms, 1000ms};
    auto program = AttemptProgram{
        {.delay = 200ms, .process_reply_result = std::nullopt},  ///< slow attempt
        {.delay = 1ms, .process_reply_result = std::nullopt},    ///< hedged request
    };

    EventLog event_log;
    engine::Yield();
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);
    EXPECT_EQ(ret, "Request1");
    EXPECT_EQ(event_log, expected_event_log);
}

UTEST(HedgedRequest, Timeout) {
    /// All requests work to long.
    /// timeout_all - 50ms
    /// every request has 100ms timing.
    /// No ProcessReply
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kStartRequest, .attempt = 1},
        {.event = Event::kStartRequest, .attempt = 2},
        {.event = Event::kFinish, .attempt = 0},
        {.event = Event::kFinish, .attempt = 1},
        {.event = Event::kFinish, .attempt = 2},
    };
    const utils::hedging::HedgingSettings settings{3, 10ms, 50ms};
    auto program = AttemptProgram{
        {.delay = 100ms, .process_reply_result = std::nullopt},
        {.delay = 100ms, .process_reply_result = std::nullopt},
        {.delay = 100ms, .process_reply_result = std::nullopt},
    };

    EventLog event_log;
    engine::Yield();
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);
    EXPECT_EQ(ret, std::nullopt);
    EXPECT_EQ(event_log, expected_event_log);
}

UTEST(HedgedRequest, Refuse) {
    /// Test case when first attempt return fast but strategy refuted to accept,
    /// so another request was made
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kStartRequest, .attempt = 1},
        {.event = Event::kProcessReply, .attempt = 1},
        {.event = Event::kFinish, .attempt = 0},
        {.event = Event::kFinish, .attempt = 1},
    };
    const utils::hedging::HedgingSettings settings{2, 200ms, 1000ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = 10ms},
        {.delay = 1ms, .process_reply_result = std::nullopt},
    };

    // Try 10 times in case of too long sleep
    for (size_t i = 0; i < 10; ++i) {
        EventLog event_log;
        engine::Yield();
        auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);
        if (ret == "Request1" && event_log == expected_event_log) {
            return;
        }
    }
    FAIL();
}

UTEST(HedgedRequest, RetryDelayWithoutRequestsInFlight) {
    /// Test case when the only attempt in flight was refused by strategy: the
    /// next attempt must not start before the delay returned by ProcessReply
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kStartRequest, .attempt = 1},
        {.event = Event::kProcessReply, .attempt = 1},
        {.event = Event::kFinish, .attempt = 0},
        {.event = Event::kFinish, .attempt = 1},
    };
    constexpr auto kRetryDelay = 100ms;
    const utils::hedging::HedgingSettings settings{2, 2000ms, 5000ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = kRetryDelay},
        {.delay = 1ms, .process_reply_result = std::nullopt},
    };

    EventLog event_log;
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);

    EXPECT_EQ(ret, "Request1");
    ASSERT_EQ(event_log, expected_event_log);
    const auto reply_time = FindEventTime(event_log, Event::kProcessReply, 0);
    const auto retry_time = FindEventTime(event_log, Event::kStartRequest, 1);
    ASSERT_TRUE(reply_time.has_value() && retry_time.has_value());
    const auto retry_interval = retry_time.value() - reply_time.value();
    EXPECT_GE(retry_interval, kRetryDelay);
    EXPECT_LT(retry_interval, settings.hedging_delay);
}

UTEST(HedgedRequest, RetryDelayWithMockedTime) {
    /// Test case when time is mocked and frozen, as in testsuite: the retry
    /// delay must elapse in real time
    utils::datetime::MockNowSet(std::chrono::system_clock::now());
    constexpr auto kRetryDelay = 100ms;
    const utils::hedging::HedgingSettings settings{2, 2000ms, 5000ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = kRetryDelay},
        {.delay = 1ms, .process_reply_result = std::nullopt},
    };

    EventLog event_log;
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);

    EXPECT_EQ(ret, "Request1");
    const auto reply_time = FindEventTime(event_log, Event::kProcessReply, 0);
    const auto retry_time = FindEventTime(event_log, Event::kStartRequest, 1);
    ASSERT_TRUE(reply_time.has_value() && retry_time.has_value());
    const auto retry_interval = retry_time.value() - reply_time.value();
    EXPECT_GE(retry_interval, kRetryDelay);
    EXPECT_LT(retry_interval, settings.hedging_delay);
}

UTEST(HedgedRequest, HedgingWithMockedTime) {
    /// Test case when time is mocked and frozen, as in testsuite: the hedging
    /// attempt must start after hedging_delay in real time
    utils::datetime::MockNowSet(std::chrono::system_clock::now());
    const utils::hedging::HedgingSettings settings{2, 10ms, 5000ms};
    auto program = AttemptProgram{
        {.delay = 1000ms, .process_reply_result = std::nullopt},  ///< slow attempt
        {.delay = 1ms, .process_reply_result = std::nullopt},     ///< hedged request
    };

    EventLog event_log;
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);

    EXPECT_EQ(ret, "Request1");
    const auto first_start_time = FindEventTime(event_log, Event::kStartRequest, 0);
    const auto hedging_start_time = FindEventTime(event_log, Event::kStartRequest, 1);
    ASSERT_TRUE(first_start_time.has_value() && hedging_start_time.has_value());
    EXPECT_LT(hedging_start_time.value() - first_start_time.value(), 1000ms);
}

UTEST(HedgedRequest, RetryDelaysWithMockedTimeRespectTimeout) {
    /// Test case when time is mocked and frozen, as in testsuite: retry delays
    /// add up and the attempts that do not fit into timeout_all are not started
    utils::datetime::MockNowSet(std::chrono::system_clock::now());
    const utils::hedging::HedgingSettings settings{5, 2000ms, 500ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = 200ms},
        {.delay = 1ms, .process_reply_result = 200ms},
        {.delay = 1ms, .process_reply_result = 200ms},
        {.delay = 1ms, .process_reply_result = 200ms},
        {.delay = 1ms, .process_reply_result = 200ms},
    };

    EventLog event_log;
    utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);

    EXPECT_TRUE(FindEventTime(event_log, Event::kStartRequest, 2).has_value());
    EXPECT_FALSE(FindEventTime(event_log, Event::kStartRequest, 3).has_value());
}

UTEST(HedgedRequest, ReturnsRightAfterReply) {
    /// Test case when the reply is accepted: the call must not wait for the
    /// rest of the plan (hedging attempts, timeout_all)
    const utils::hedging::HedgingSettings settings{3, 2000ms, 5000ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = std::nullopt},
    };

    EventLog event_log;
    const auto start = std::chrono::steady_clock::now();
    auto ret = utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(ret, "Request0");
    EXPECT_LT(elapsed, 1000ms);
}

UTEST(HedgedRequest, ReturnsRightAfterAttemptsExhausted) {
    /// Test case when all attempts were refused by strategy: the call must not
    /// wait for timeout_all
    const utils::hedging::HedgingSettings settings{2, 2000ms, 5000ms};
    auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = 10ms},
        {.delay = 1ms, .process_reply_result = 10ms},
    };

    EventLog event_log;
    const auto start = std::chrono::steady_clock::now();
    utils::hedging::HedgeRequest(TestStrategy(event_log, program), settings);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, 1000ms);
}

UTEST(HedgedRequest, RetryDelayPostponesHedging) {
    /// Test case when the first attempt was refused by strategy: the hedging
    /// attempt is not started before the delay returned by ProcessReply
    constexpr auto kRetryDelay = 600ms;
    const utils::hedging::HedgingSettings settings{2, 200ms, 5000ms};
    ControlledAttempts attempts;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(1));
    attempts.promises[0].set_value(kRetryDelay);
    ASSERT_TRUE(attempts.WaitForReplied(1));
    ASSERT_LT(attempts.reply_times.at(0) - attempts.create_times[0], settings.hedging_delay);

    ASSERT_TRUE(attempts.WaitForCreated(2));
    EXPECT_GE(attempts.create_times[1] - attempts.reply_times.at(0), kRetryDelay);

    attempts.promises[1].set_value(std::nullopt);
    EXPECT_EQ(future.Get(), "Request1");
}

UTEST(HedgedRequest, RetryDelayBeyondTimeout) {
    /// Test case when the delay returned by ProcessReply leaves no time for
    /// another attempt: the call returns without waiting for timeout_all
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kFinish, .attempt = 0},
    };
    const utils::hedging::HedgingSettings settings{2, 2000ms, 3000ms};
    const auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = 10000ms},
    };

    EventLog event_log;
    const auto elapsed = GetDuration(settings, TestStrategy(event_log, program));

    EXPECT_EQ(event_log, expected_event_log);
    EXPECT_LT(elapsed, 1000ms);
}

UTEST(HedgedRequest, ExampleStrategy) {
    const utils::hedging::HedgingSettings settings{3, 1000ms, 5000ms};
    const auto ret = utils::hedging::HedgeRequest(ExampleStrategy(1), settings);
    EXPECT_EQ(ret, "attempt 1");
}

UTEST(HedgedRequest, ZeroRetryDelay) {
    /// Test case when the strategy asks to retry immediately: the next attempt
    /// is not postponed till hedging
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kStartRequest, .attempt = 1},
        {.event = Event::kProcessReply, .attempt = 1},
        {.event = Event::kFinish, .attempt = 0},
        {.event = Event::kFinish, .attempt = 1},
    };
    const utils::hedging::HedgingSettings settings{2, 2000ms, 5000ms};
    const auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = 0ms},
        {.delay = 1ms, .process_reply_result = std::nullopt},
    };

    EventLog event_log;
    const auto elapsed = GetDuration(settings, TestStrategy(event_log, program));

    EXPECT_EQ(event_log, expected_event_log);
    EXPECT_LT(elapsed, 1000ms);
}

UTEST(HedgedRequest, RetryDelayWithAttemptInFlight) {
    /// Test case when an attempt was refused by strategy while another attempt
    /// is in flight: the next attempt is not started before the delay
    constexpr auto kRetryDelay = 500ms;
    const utils::hedging::HedgingSettings settings{3, 100ms, 5000ms};
    ControlledAttempts attempts;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(2));
    attempts.promises[0].set_value(kRetryDelay);

    ASSERT_TRUE(attempts.WaitForCreated(3));
    EXPECT_GE(attempts.create_times[2] - attempts.reply_times.at(0), kRetryDelay);

    attempts.promises[2].set_value(std::nullopt);
    EXPECT_EQ(future.Get(), "Request2");
    EXPECT_EQ(attempts.process_reply_calls, 2);
}

UTEST(HedgedRequest, ShorterRetryDelayDoesNotShortenLonger) {
    constexpr auto kLongRetryDelay = 500ms;
    const utils::hedging::HedgingSettings settings{3, 100ms, 5000ms};
    ControlledAttempts attempts;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(2));
    attempts.promises[0].set_value(kLongRetryDelay);
    attempts.promises[1].set_value(1ms);

    ASSERT_TRUE(attempts.WaitForCreated(3));
    EXPECT_GE(attempts.create_times[2] - attempts.reply_times.at(0), kLongRetryDelay);

    attempts.promises[2].set_value(std::nullopt);
    EXPECT_EQ(future.Get(), "Request2");
}

UTEST(HedgedRequest, LongerRetryDelayExtendsShorter) {
    constexpr auto kLongRetryDelay = 500ms;
    const utils::hedging::HedgingSettings settings{3, 100ms, 5000ms};
    ControlledAttempts attempts;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(2));
    attempts.promises[0].set_value(1ms);
    attempts.promises[1].set_value(kLongRetryDelay);

    ASSERT_TRUE(attempts.WaitForCreated(3));
    EXPECT_GE(attempts.create_times[2] - attempts.reply_times.at(1), kLongRetryDelay);

    attempts.promises[2].set_value(std::nullopt);
    EXPECT_EQ(future.Get(), "Request2");
}

UTEST(HedgedRequest, MaxRetryDelay) {
    /// Test case when the delay is too big to be added to the current time
    const EventLog expected_event_log = {
        {.event = Event::kStartRequest, .attempt = 0},
        {.event = Event::kProcessReply, .attempt = 0},
        {.event = Event::kFinish, .attempt = 0},
    };
    const utils::hedging::HedgingSettings settings{2, 2000ms, 3000ms};
    const auto program = AttemptProgram{
        {.delay = 1ms, .process_reply_result = std::chrono::milliseconds::max()},
    };

    EventLog event_log;
    const auto elapsed = GetDuration(settings, TestStrategy(event_log, program));

    EXPECT_EQ(event_log, expected_event_log);
    EXPECT_LT(elapsed, 1000ms);
}

UTEST(HedgedRequest, RetryDelayBeyondTimeoutKeepsAttemptInFlight) {
    /// Test case when the delay leaves no time for another attempt, but the
    /// attempt in flight succeeds: its reply is returned
    const utils::hedging::HedgingSettings settings{3, 200ms, 1000ms};
    ControlledAttempts attempts;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(2));
    attempts.promises[0].set_value(10000ms);
    ASSERT_TRUE(attempts.WaitForReplied(1));

    attempts.promises[1].set_value(std::nullopt);
    EXPECT_EQ(future.Get(), "Request1");
    EXPECT_EQ(attempts.create_calls, 2);
}

UTEST(HedgedRequest, RetryDelayDoesNotPostponeOtherRequests) {
    constexpr auto kRetryDelay = 1000ms;
    const utils::hedging::HedgingSettings settings{2, 50ms, 5000ms};
    ControlledAttempts delayed_attempts;
    ControlledAttempts hedged_attempts;
    std::vector<ControlledStrategy> strategies;
    strategies.emplace_back(delayed_attempts);
    strategies.emplace_back(hedged_attempts);
    auto future = utils::hedging::HedgeRequestsBulkAsync(std::move(strategies), settings);

    ASSERT_TRUE(delayed_attempts.WaitForCreated(1));
    delayed_attempts.promises[0].set_value(kRetryDelay);

    ASSERT_TRUE(hedged_attempts.WaitForCreated(2));
    EXPECT_LT(hedged_attempts.create_times[1] - hedged_attempts.create_times[0], kRetryDelay / 2);
    hedged_attempts.promises[1].set_value(std::nullopt);

    ASSERT_TRUE(delayed_attempts.WaitForCreated(2));
    EXPECT_GE(delayed_attempts.create_times[1] - delayed_attempts.reply_times.at(0), kRetryDelay);
    delayed_attempts.promises[1].set_value(std::nullopt);

    const std::vector<std::optional<std::string>> expected_replies{"Request1", "Request1"};
    EXPECT_EQ(future.Get(), expected_replies);
}

UTEST(HedgedRequest, CancelWhileWaitingForRetryDelay) {
    const utils::hedging::HedgingSettings settings{2, 3000ms, 10000ms};
    ControlledAttempts attempts;
    std::optional future{utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings)};

    ASSERT_TRUE(attempts.WaitForCreated(1));
    attempts.promises[0].set_value(2000ms);
    ASSERT_TRUE(attempts.WaitForReplied(1));

    const auto start = std::chrono::steady_clock::now();
    future.reset();
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1000ms);
    EXPECT_EQ(attempts.create_calls, 1);
}

UTEST(HedgedRequest, CreateRefusesAfterRetryDelay) {
    /// Test case when the strategy refuses to create the attempt planned by
    /// the delay: the call returns without waiting for timeout_all
    const utils::hedging::HedgingSettings settings{3, 2000ms, 5000ms};
    ControlledAttempts attempts;
    attempts.attempts_limit = 1;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(1));
    const auto start = std::chrono::steady_clock::now();
    attempts.promises[0].set_value(50ms);

    EXPECT_EQ(future.Get(), std::nullopt);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1000ms);
    EXPECT_EQ(attempts.create_calls, 2);
}

UTEST(HedgedRequest, HedgingResumesAfterRetryDelay) {
    constexpr auto kRetryDelay = 300ms;
    const utils::hedging::HedgingSettings settings{3, 200ms, 5000ms};
    ControlledAttempts attempts;
    auto future = utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings);

    ASSERT_TRUE(attempts.WaitForCreated(1));
    attempts.promises[0].set_value(kRetryDelay);

    ASSERT_TRUE(attempts.WaitForCreated(3));
    EXPECT_GE(attempts.create_times[1] - attempts.reply_times.at(0), kRetryDelay);
    EXPECT_GE(attempts.create_times[2] - attempts.reply_times.at(0), kRetryDelay + settings.hedging_delay);
    EXPECT_LT(attempts.create_times[2] - attempts.create_times[1], settings.hedging_delay + 1000ms);

    attempts.promises[2].set_value(std::nullopt);
    EXPECT_EQ(future.Get(), "Request2");
}

UTEST(HedgedRequest, CancelWithAttemptsInFlight) {
    const utils::hedging::HedgingSettings settings{2, 50ms, 10000ms};
    ControlledAttempts attempts;
    std::optional future{utils::hedging::HedgeRequestAsync(ControlledStrategy(attempts), settings)};

    ASSERT_TRUE(attempts.WaitForCreated(2));

    const auto start = std::chrono::steady_clock::now();
    future.reset();
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1000ms);
    EXPECT_EQ(attempts.finish_calls, 2);
    EXPECT_EQ(attempts.process_reply_calls, 0);
}

UTEST(HedgedRequest, ZeroMaxAttempts) {
    const utils::hedging::HedgingSettings settings{0, 10ms, 5000ms};
    ControlledAttempts attempts;

    const auto start = std::chrono::steady_clock::now();
    const auto ret = utils::hedging::HedgeRequest(ControlledStrategy(attempts), settings);

    EXPECT_EQ(ret, std::nullopt);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1000ms);
    EXPECT_EQ(attempts.create_calls, 0);
}

UTEST(HedgedRequest, EmptyBulk) {
    const utils::hedging::HedgingSettings settings{3, 10ms, 5000ms};

    const auto start = std::chrono::steady_clock::now();
    const auto replies = utils::hedging::HedgeRequestsBulk(std::vector<ControlledStrategy>{}, settings);

    EXPECT_TRUE(replies.empty());
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1000ms);
}

USERVER_NAMESPACE_END
