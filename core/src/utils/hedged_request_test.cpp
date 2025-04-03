#include <userver/utils/hedged_request.hpp>

#include <chrono>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kDefaultDelay = 100ms;

struct TestFuture {
    TestFuture(std::string reply, size_t attempt, std::chrono::milliseconds delay)
        : task(utils::Async(
              "test",
              [reply, delay] {
                  engine::SleepFor(delay);
                  return reply;
              }
          )),
          attempt(attempt) {}
    TestFuture(TestFuture&&) noexcept = default;
    std::string Get() { return task.Get(); }
    auto* TryGetContextAccessor() { return task.TryGetContextAccessor(); }

    engine::TaskWithResult<std::string> task;
    size_t attempt{0};
};

enum class Event { StartRequest, ProcessReply, Finish };
std::string_view ToString(Event event) {
    switch (event) {
        case Event::StartRequest:
            return "StartRequest";
        case Event::ProcessReply:
            return "ProcessReply";
        case Event::Finish:
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
        : attempt_program(std::move(attempt_program)), event_log(event_log) {}

    std::optional<RequestType> Create(size_t attempt) {
        event_log.push_back({Event::StartRequest, attempt});
        auto delay = kDefaultDelay;
        if (attempt_program.size() > attempt) {
            delay = attempt_program[attempt].delay;
        }
        return TestFuture("Request" + std::to_string(attempt), attempt, delay);
    }

    std::optional<std::chrono::milliseconds> ProcessReply(RequestType&& request) {
        reply_ = std::move(request).Get();
        event_log.push_back({Event::ProcessReply, request.attempt});
        if (attempt_program.size() <= request.attempt) return std::nullopt;
        return attempt_program[request.attempt].process_reply_result;
    }

    std::optional<ReplyType> ExtractReply() { return reply_; }

    void Finish(RequestType&& request) {
        event_log.push_back({Event::Finish, request.attempt});
        if (request.task.IsValid()) request.task.SyncCancel();
    }

    std::optional<ReplyType> reply_;
    AttemptProgram attempt_program;
    EventLog& event_log;
};

}  // namespace

UTEST(HedgedRequest, Base) {
    /// Test basic case:
    /// make 3 consecutive requests with 10 ms delay. Get reply from first attempt
    /// and cancel all requests
    const EventLog expected_event_log = {
        {Event::StartRequest, 0},
        {Event::StartRequest, 1},
        {Event::StartRequest, 2},
        {Event::ProcessReply, 0},
        {Event::Finish, 0},
        {Event::Finish, 1},
        {Event::Finish, 2},
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
        {Event::StartRequest, 0},
        {Event::ProcessReply, 0},
        {Event::Finish, 0},
    };
    const utils::hedging::HedgingSettings settings{3, 200ms, 1000ms};
    auto program = AttemptProgram{
        {1ms, std::nullopt},
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
        {Event::StartRequest, 0},
        {Event::StartRequest, 1},
        {Event::ProcessReply, 1},
        {Event::Finish, 0},
        {Event::Finish, 1},
    };
    const utils::hedging::HedgingSettings settings{2, 10ms, 1000ms};
    auto program = AttemptProgram{
        {200ms, std::nullopt},  ///< slow attempt
        {1ms, std::nullopt},    ///< hedged request
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
        {Event::StartRequest, 0},
        {Event::StartRequest, 1},
        {Event::StartRequest, 2},
        {Event::Finish, 0},
        {Event::Finish, 1},
        {Event::Finish, 2},
    };
    const utils::hedging::HedgingSettings settings{3, 10ms, 50ms};
    auto program = AttemptProgram{
        {100ms, std::nullopt},
        {100ms, std::nullopt},
        {100ms, std::nullopt},
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
        {Event::StartRequest, 0},
        {Event::ProcessReply, 0},
        {Event::StartRequest, 1},
        {Event::ProcessReply, 1},
        {Event::Finish, 0},
        {Event::Finish, 1},
    };
    const utils::hedging::HedgingSettings settings{2, 200ms, 1000ms};
    auto program = AttemptProgram{
        {1ms, 10ms},
        {1ms, std::nullopt},
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

USERVER_NAMESPACE_END
