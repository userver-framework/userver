#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <userver/clients/http/cancellation_policy.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/concurrent/collect_in_parallel.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/io/tests/net_listener.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {
namespace {

using namespace std::chrono_literals;

class LookupError : public std::runtime_error {
public:
    LookupError() : std::runtime_error("lookup failed") {}
};

UTEST(CollectInParallel, EmptyInputAndZeroLimit) {
    int calls = 0;
    const auto start = [&calls](int input) {
        ++calls;
        return utils::Async("unused", [input] { return input; });
    };
    const std::vector<int> empty;
    EXPECT_TRUE(CollectInParallel(empty, {.max_parallel_tasks = 2}, start).empty());
    EXPECT_THROW(CollectInParallel(empty, {.max_parallel_tasks = 0}, start), std::invalid_argument);
    EXPECT_EQ(calls, 0);
}

UTEST(CollectInParallel, CancellationBeforeStartThrowsStandardException) {
    const std::vector<int> inputs{1};
    int launches = 0;
    const auto start = [&launches](int input) {
        ++launches;
        return utils::Async("not-started", [input] { return input; });
    };
    engine::current_task::RequestCancel();
    EXPECT_THROW(CollectInParallel(inputs, {.max_parallel_tasks = 1}, start), engine::WaitInterruptedException);
    EXPECT_EQ(launches, 0);
}

UTEST(CollectInParallel, MoveOnlyResultsAndDuplicateInputs) {
    /// [sample CollectInParallel tasks]
    const std::vector<int> inputs{7, 7, 3};
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [](int input) {
        return utils::Async("move-only", [input] { return std::make_unique<int>(input * 2); });
    });
    ASSERT_EQ(results.size(), inputs.size());
    int sevens = 0;
    for (std::size_t index = 0; index < results.size(); ++index) {
        ASSERT_TRUE(results[index].has_value());
        EXPECT_EQ(*results[index].value(), inputs[index] * 2);
        if (inputs[index] == 7) {
            ++sevens;
        }
    }
    EXPECT_EQ(sevens, 2);
    /// [sample CollectInParallel tasks]
}

UTEST(CollectInParallel, CapturesStartAndTaskFailuresWithoutRetries) {
    /// [sample CollectInParallel continue]
    const std::vector<int> inputs{0, 1, 2, 3};
    std::array<int, 4> calls{};
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [&calls](int input) {
        ++calls[input];
        if (input == 1) {
            throw LookupError{};
        }
        return utils::Async("possibly-failing", [input] {
            if (input == 2) {
                throw LookupError{};
            }
            return input * 2;
        });
    });
    ASSERT_EQ(results.size(), inputs.size());
    for (std::size_t index = 0; index < results.size(); ++index) {
        const auto input = inputs[index];
        const auto& result = results[index];
        EXPECT_EQ(calls[input], 1);
        if (input == 1 || input == 2) {
            ASSERT_FALSE(result.has_value());
            EXPECT_THROW(std::rethrow_exception(result.error()), LookupError);
        } else {
            ASSERT_TRUE(result.has_value());
            EXPECT_EQ(result.value(), input * 2);
        }
    }
    /// [sample CollectInParallel continue]
}

UTEST(CollectInParallel, SupportsVoidResults) {
    const std::vector<int> inputs{0, 1, 2};
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 1}, [](int input) {
        return utils::Async("void", [input] {
            if (input == 1) {
                throw LookupError{};
            }
        });
    });
    ASSERT_EQ(results.size(), inputs.size());
    for (std::size_t index = 0; index < results.size(); ++index) {
        EXPECT_EQ(results[index].has_value(), inputs[index] != 1);
    }
}

UTEST(CollectInParallel, NonCopyableInputsPassedByReference) {
    std::array inputs{std::make_unique<int>(1), std::make_unique<int>(2)};
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [](std::unique_ptr<int>& input) {
        const auto value = ++*input;
        return utils::Async("non-copyable-input", [value] { return value; });
    });
    ASSERT_EQ(results.size(), inputs.size());
    EXPECT_EQ(results[0].value(), 2);
    EXPECT_EQ(results[1].value(), 3);
    EXPECT_EQ(*inputs[0], 2);
    EXPECT_EQ(*inputs[1], 3);
}

UTEST(CollectInParallel, ForwardsRvalueReferences) {
    std::array inputs{std::make_unique<int>(1), std::make_unique<int>(2)};
    auto range = inputs | std::views::transform([](auto& input) -> auto&& { return std::move(input); });
    const auto results = CollectInParallel(range, {.max_parallel_tasks = 2}, [](std::unique_ptr<int>&& input) {
        return utils::Async("moved-input", [input = std::move(input)] { return *input; });
    });
    ASSERT_EQ(results.size(), inputs.size());
    EXPECT_EQ(results[0].value(), 1);
    EXPECT_EQ(results[1].value(), 2);
    EXPECT_FALSE(inputs[0]);
    EXPECT_FALSE(inputs[1]);
}

UTEST(CollectInParallel, SupportsProxyReferences) {
    std::vector<bool> inputs{false, true};
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [](std::vector<bool>::reference input) {
        input = !input;
        return utils::Async("proxy-input", [value = static_cast<bool>(input)] { return value; });
    });
    ASSERT_EQ(results.size(), inputs.size());
    EXPECT_TRUE(results[0].value());
    EXPECT_FALSE(results[1].value());
    EXPECT_TRUE(inputs[0]);
    EXPECT_FALSE(inputs[1]);
}

UTEST(CollectInParallel, SupportsSinglePassRange) {
    std::istringstream stream{"3 1 4"};
    auto inputs = std::ranges::istream_view<int>(stream);
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [](int input) {
        return utils::Async("single-pass-input", [input] { return input * 2; });
    });
    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].value(), 6);
    EXPECT_EQ(results[1].value(), 2);
    EXPECT_EQ(results[2].value(), 8);
}

UTEST(CollectInParallel, EvaluatesLazyMoveOnlyInputsOnce) {
    int evaluations = 0;
    auto inputs =
        std::views::iota(0, 3) | std::views::transform([&evaluations](int value) {
            ++evaluations;
            return std::make_unique<int>(value);
        });
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [](std::unique_ptr<int> input) {
        return utils::Async("lazy-input", [input = std::move(input)] { return *input; });
    });
    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(evaluations, 3);
    for (std::size_t index = 0; index < results.size(); ++index) {
        EXPECT_EQ(results[index].value(), index);
    }
}

UTEST(CollectInParallel, FutureWindowReplenishedInCompletionOrder) {
    const std::vector<int> inputs{0, 1, 2, 3};
    std::array<engine::Promise<int>, 4> promises;
    std::array<engine::SingleConsumerEvent, 4> started;
    auto collector = utils::Async("collect-futures", [&inputs, &promises, &started] {
        return CollectInParallel(inputs, {.max_parallel_tasks = 2}, [&promises, &started](int input) {
            started[input].Send();
            return promises[input].get_future();
        });
    });
    ASSERT_TRUE(started[0].WaitForEventFor(5s));
    ASSERT_TRUE(started[1].WaitForEventFor(5s));
    EXPECT_FALSE(started[2].WaitForEventFor(20ms));
    promises[1].set_value(10);
    ASSERT_TRUE(started[2].WaitForEventFor(5s));
    EXPECT_FALSE(started[3].WaitForEventFor(20ms));
    promises[2].set_value(20);
    ASSERT_TRUE(started[3].WaitForEventFor(5s));
    promises[0].set_value(0);
    promises[3].set_value(30);
    const auto results = collector.Get();
    ASSERT_EQ(results.size(), inputs.size());
    for (std::size_t index = 0; index < results.size(); ++index) {
        ASSERT_TRUE(results[index].has_value());
        EXPECT_EQ(results[index].value(), inputs[index] * 10);
    }
}

UTEST(CollectInParallel, ParentCancellationPropagatesAndJoinsTasks) {
    const std::vector<int> inputs{0, 1, 2, 3};
    std::array<engine::SingleConsumerEvent, 2> started;
    std::array<engine::SingleConsumerEvent, 2> finish;
    std::atomic<int> active{0};
    std::atomic<int> launches{0};
    auto collector = utils::Async("cancel-collector", [&inputs, &started, &finish, &active, &launches] {
        return CollectInParallel(inputs, {.max_parallel_tasks = 2}, [&started, &finish, &active, &launches](int input) {
            ++launches;
            return utils::Async("pending", [input, &started, &finish, &active] {
                ++active;
                const utils::FastScopeGuard cleanup{[&active]() noexcept { --active; }};
                started.at(input).Send();
                if (!finish.at(input).WaitForEvent()) {
                    engine::current_task::CancellationPoint();
                }
                return input;
            });
        });
    });
    ASSERT_TRUE(started[0].WaitForEventFor(5s));
    ASSERT_TRUE(started[1].WaitForEventFor(5s));
    collector.SyncCancel();
    EXPECT_ANY_THROW(collector.Get());
    EXPECT_EQ(active.load(), 0);
    EXPECT_EQ(launches.load(), 2);
}

UTEST(CollectInParallel, NonStandardExceptionsPropagate) {
    const std::vector<int> inputs{1};
    engine::Promise<int> promise;
    promise.set_exception(std::make_exception_ptr(42));
    const auto start = [&promise](int) { return promise.get_future(); };
    EXPECT_THROW(CollectInParallel(inputs, {.max_parallel_tasks = 1}, start), int);
}

UTEST(CollectInParallel, FailFastOnStartErrorSkipsRemainingInputs) {
    /// [sample CollectInParallel fail fast]
    const std::vector<int> inputs{0, 1, 2, 3};
    engine::Promise<int> promise;
    promise.set_value(0);
    int launches = 0;
    const auto start = [&promise, &launches](int input) {
        ++launches;
        if (input == 1) {
            throw LookupError{};
        }
        return promise.get_future();
    };
    EXPECT_THROW(
        CollectInParallel(inputs, {.max_parallel_tasks = 2, .failure_policy = CollectFailurePolicy::kFailFast}, start),
        LookupError
    );
    EXPECT_EQ(launches, 2);
    /// [sample CollectInParallel fail fast]
}

UTEST(CollectInParallel, FailFastCancelsAndJoinsPendingTasks) {
    const std::vector<int> inputs{0, 1, 2, 3};
    engine::SingleConsumerEvent started;
    engine::SingleConsumerEvent finish;
    std::atomic<int> active{0};
    int launches = 0;
    const auto start = [&started, &finish, &active, &launches](int input) {
        ++launches;
        return utils::Async("fail-fast-task", [input, &started, &finish, &active] {
            if (input == 1) {
                EXPECT_TRUE(started.WaitForEventFor(5s));
                throw LookupError{};
            }
            ++active;
            const utils::FastScopeGuard cleanup{[&active]() noexcept { --active; }};
            started.Send();
            if (!finish.WaitForEvent()) {
                engine::current_task::CancellationPoint();
            }
            return input;
        });
    };
    EXPECT_THROW(
        CollectInParallel(inputs, {.max_parallel_tasks = 2, .failure_policy = CollectFailurePolicy::kFailFast}, start),
        LookupError
    );
    EXPECT_EQ(launches, 2);
    EXPECT_EQ(active.load(), 0);
}

UTEST(CollectInParallel, FailFastWaitsForNonCancellableFuturesAndPreservesError) {
    struct FutureWithoutWait {
        engine::Future<int> future;
        std::atomic<int>& gets;

        engine::AwaitableToken GetAwaitableToken() { return future.GetAwaitableToken(); }
        int Get() {
            ++gets;
            return future.get();
        }
    };
    const std::vector<int> inputs{0, 1, 2, 3};
    std::array<engine::Promise<int>, 2> promises;
    promises[1].set_exception(std::make_exception_ptr(LookupError{}));
    engine::SingleConsumerEvent started;
    engine::SingleConsumerEvent finished;
    std::atomic<int> launches{0};
    std::atomic<int> gets{0};
    auto collector = utils::Async("fail-fast-futures", [&inputs, &promises, &started, &finished, &launches, &gets] {
        const auto start = [&promises, &started, &launches, &gets](int input) {
            ++launches;
            if (input == 1) {
                started.Send();
            }
            return FutureWithoutWait{.future = promises.at(input).get_future(), .gets = gets};
        };
        EXPECT_THROW(
            CollectInParallel(
                inputs,
                {.max_parallel_tasks = 2, .failure_policy = CollectFailurePolicy::kFailFast},
                start
            ),
            LookupError
        );
        finished.Send();
    });
    ASSERT_TRUE(started.WaitForEventFor(5s));
    EXPECT_FALSE(finished.WaitForEventFor(20ms));
    EXPECT_EQ(launches.load(), 2);
    promises[0].set_exception(std::make_exception_ptr(std::runtime_error("cleanup failure")));
    collector.Get();
    EXPECT_EQ(launches.load(), 2);
    EXPECT_EQ(gets.load(), 1);  // Only retrieve the first failure, not the pending operation's result.
}

UTEST(CollectInParallel, FailFastBlocksParentCancellationDuringCleanup) {
    struct FutureWithCancellationRequest {
        engine::Future<int> future;
        engine::SingleConsumerEvent& cancel_requested;

        engine::AwaitableToken GetAwaitableToken() { return future.GetAwaitableToken(); }
        int Get() { return future.get(); }
        void RequestCancel() { cancel_requested.Send(); }
    };
    const std::vector<int> inputs{0, 1, 2};
    std::array<engine::Promise<int>, 2> promises;
    promises[1].set_exception(std::make_exception_ptr(LookupError{}));
    engine::SingleConsumerEvent cancel_requested;
    engine::SingleConsumerEvent finished;
    auto collector = utils::Async("cancel-during-cleanup", [&inputs, &promises, &cancel_requested, &finished] {
        const utils::FastScopeGuard cleanup{[&finished]() noexcept { finished.Send(); }};
        return CollectInParallel(
            inputs,
            {.max_parallel_tasks = 2, .failure_policy = CollectFailurePolicy::kFailFast},
            [&promises, &cancel_requested](int input) {
                return FutureWithCancellationRequest{
                    .future = promises.at(input).get_future(),
                    .cancel_requested = cancel_requested,
                };
            }
        );
    });
    ASSERT_TRUE(cancel_requested.WaitForEventFor(5s));
    collector.RequestCancel();
    EXPECT_FALSE(finished.WaitForEventFor(20ms));
    promises[0].set_value(0);
    EXPECT_THROW(collector.Get(), LookupError);
}

UTEST(CollectInParallel, FailFastCancelsInvalidatingFuturesWithoutGettingThem) {
    struct CancellableFuture {
        engine::Future<int> future;
        engine::Promise<int>& promise;
        int& cancellations;

        engine::AwaitableToken GetAwaitableToken() { return future.GetAwaitableToken(); }
        int Get() { return future.get(); }
        void Cancel() {
            ++cancellations;
            // Cancellation completes the operation and notifies its existing awaiters.
            promise.set_exception(std::make_exception_ptr(std::runtime_error("cancelled")));
            future = {};
        }
    };
    const std::vector<int> inputs{0, 1, 2};
    std::array<engine::Promise<int>, 2> promises;
    promises[1].set_exception(std::make_exception_ptr(LookupError{}));
    int cancellations = 0;
    int launches = 0;
    const auto start = [&promises, &cancellations, &launches](int input) {
        ++launches;
        return CancellableFuture{
            .future = promises.at(input).get_future(),
            .promise = promises.at(input),
            .cancellations = cancellations,
        };
    };
    EXPECT_THROW(
        CollectInParallel(inputs, {.max_parallel_tasks = 2, .failure_policy = CollectFailurePolicy::kFailFast}, start),
        LookupError
    );
    EXPECT_EQ(launches, 2);
    EXPECT_EQ(cancellations, 1);
}

UTEST(CollectInParallel, SupportsHttpResponseFutures) {
    const utest::SimpleServer server{[](const utest::SimpleServer::Request&) {
        return utest::SimpleServer::Response{
            .data_to_send = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok",
        };
    }};
    auto client = utest::CreateHttpClient();
    const std::vector<int> inputs{0, 1, 2};
    const auto results = CollectInParallel(inputs, {.max_parallel_tasks = 2}, [&client, &server](int) {
        return client->CreateRequest()
            .get(server.GetBaseUrl())
            .retry(1)
            .timeout(utest::kMaxTestWaitTime)
            .async_perform();
    });
    ASSERT_EQ(results.size(), inputs.size());
    for (const auto& result : results) {
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result.value()->IsOk());
        EXPECT_EQ(result.value()->body_view(), "ok");
    }
}

UTEST(CollectInParallel, FailFastExplicitlyCancelsHttpResponseFuture) {
    engine::io::tests::TcpListener listener{engine::io::tests::IpVersion::kV4};
    engine::SingleConsumerEvent request_received;
    auto server = utils::Async("observe-http-cancellation", [&listener, &request_received] {
        const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
        auto peer = listener.socket.Accept(deadline);
        std::array<char, 1024> buffer{};
        std::string request;
        while (!request.ends_with("\r\n\r\n")) {
            const auto size = peer.RecvSome(buffer.data(), buffer.size(), deadline);
            ASSERT_GT(size, 0);
            request.append(buffer.data(), size);
        }
        request_received.Send();
        // No response is sent. Explicit cancellation must close the connection.
        EXPECT_EQ(peer.RecvSome(buffer.data(), buffer.size(), deadline), 0);
    });
    auto client = utest::CreateHttpClient();
    const auto url = "http://127.0.0.1:" + std::to_string(listener.Port());
    const std::vector<int> inputs{0, 1, 2};
    int launches = 0;
    const auto start = [&client, &url, &request_received, &launches](int input) {
        ++launches;
        if (input == 1) {
            EXPECT_TRUE(request_received.WaitForEventFor(utest::kMaxTestWaitTime));
            throw LookupError{};
        }
        auto request = client->CreateRequest().get(url).retry(1).timeout(utest::kMaxTestWaitTime * 10);
        // Destruction alone would detach. This test requires the collector to call Cancel().
        request.SetCancellationPolicy(clients::http::CancellationPolicy::kIgnore);
        return request.async_perform();
    };
    EXPECT_THROW(
        CollectInParallel(inputs, {.max_parallel_tasks = 2, .failure_policy = CollectFailurePolicy::kFailFast}, start),
        LookupError
    );
    EXPECT_EQ(launches, 2);
    server.Get();
}

}  // namespace
}  // namespace concurrent

USERVER_NAMESPACE_END
