#pragma once

/// @file userver/concurrent/collect_in_parallel.hpp
/// @brief @copybrief concurrent::CollectInParallel

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <userver/engine/awaitable.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/expected.hpp>
#include <userver/utils/invoke_with_expected.hpp>
#include <userver/utils/required.hpp>
#include <userver/utils/slot_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

enum class CollectFailurePolicy : std::uint8_t {
    kContinue,  ///< Collect individual errors and process all inputs.
    kFailFast,  ///< Stop starting inputs, clean up in-flight work, and rethrow the first observed error.
};

/// @brief Settings for @ref concurrent::CollectInParallel.
struct ParallelCollectSettings final {
    /// Maximum number of in-flight operations. Must be explicitly set to a positive value.
    utils::Required<std::size_t> max_parallel_tasks;
    /// Whether to collect individual errors or stop on the first observed error.
    CollectFailurePolicy failure_policy{CollectFailurePolicy::kContinue};
};

namespace impl {

template <typename Future>
concept CollectableFuture =
    std::is_object_v<Future> && std::move_constructible<Future> && engine::Awaitable<Future> &&
    (requires(Future& future) { future.Get(); } || requires(Future& future) { future.get(); });

template <CollectableFuture Future>
decltype(auto) GetCollectedValue(Future& future) {
    if constexpr (requires { future.Get(); }) {
        return future.Get();
    } else {
        return future.get();
    }
}

template <CollectableFuture Future>
using CollectedValue = std::decay_t<decltype(GetCollectedValue(std::declval<Future&>()))>;

inline std::uint64_t WaitForCollectedTask(engine::WaitAnyContext& wait_any) {
    const auto ready = wait_any.Wait();
    if (ready.has_value()) {
        return ready.value();
    }
    switch (ready.error()) {
        case engine::WaitAnyError::kCancelled:
            engine::current_task::CancellationPointWeak();
            throw std::logic_error("Parallel collector wait interrupted without pending cancellation");
        case engine::WaitAnyError::kEmpty:
            throw std::logic_error("Parallel collector has no in-flight operations");
        case engine::WaitAnyError::kTimeout:
            throw std::logic_error("Parallel collector unexpectedly timed out");
    }
    throw std::logic_error("Unknown parallel collector wait error");
}

template <CollectableFuture Future>
auto CollectResult(Future& future) {
    auto result = utils::InvokeWithExpected([&future]() -> decltype(auto) { return GetCollectedValue(future); });
    if (!result) {
        engine::current_task::CancellationPointWeak();
    }
    return result;
}

template <typename InputReference, typename Start>
class ParallelCollector final {
    using Future = std::invoke_result_t<const Start&, InputReference>;
    using Outcome = utils::expected<CollectedValue<Future>, std::exception_ptr>;
    struct InFlight {
        std::size_t result_index{};
        std::optional<Future> future{};
    };

public:
    ParallelCollector(std::size_t concurrency, const Start& start, CollectFailurePolicy failure_policy)
        : concurrency_(concurrency),
          start_(start),
          failure_policy_(failure_policy)
    {}

    template <std::ranges::input_range Range>
    std::vector<Outcome> Run(Range&& range) {
        if constexpr (std::ranges::sized_range<Range>) {
            results_.reserve(std::ranges::size(range));
        }
        for (auto&& input : range) {
            engine::current_task::CancellationPointWeak();
            StartOne(std::forward<decltype(input)>(input));
            if (tasks_.size() < concurrency_) {
                continue;
            }
            CollectOne();
        }
        while (!tasks_.empty()) {
            CollectOne();
        }
        return TakeResults();
    }

private:
    void StartOne(InputReference&& input) {
        const auto result_index = results_.size();
        results_.emplace_back();
        auto [request, index] = tasks_.emplace(InFlight{.result_index = result_index});
        auto started = utils::InvokeWithExpected([this, &request, &input] {
            request.future.emplace(std::invoke(start_, std::forward<InputReference>(input)));
        });
        if (!started) {
            engine::current_task::CancellationPointWeak();
            auto error = std::move(started.error());
            if (failure_policy_ == CollectFailurePolicy::kFailFast) {
                tasks_.erase(index);
                CancelAndWait();
                std::rethrow_exception(error);
            }
            results_[result_index].emplace(utils::unexpected(std::move(error)));
            tasks_.erase(index);
            return;
        }
        wait_any_.Append(index, request.future.value());
    }

    void CollectOne() {
        const auto index = WaitForCollectedTask(wait_any_);
        auto& request = tasks_[index];
        auto result = CollectResult(request.future.value());
        if (!result && failure_policy_ == CollectFailurePolicy::kFailFast) {
            const auto error = std::move(result.error());
            tasks_.erase(index);
            CancelAndWait();
            std::rethrow_exception(error);
        }
        results_[request.result_index].emplace(std::move(result));
        tasks_.erase(index);
    }

    std::vector<Outcome> TakeResults() {
        std::vector<Outcome> results;
        results.reserve(results_.size());
        for (auto& result : results_) {
            results.push_back(std::move(result).value());
        }
        return results;
    }

    void CancelPending() {
        if constexpr (
            !requires(Future& future) { future.RequestCancel(); } &&
            !requires(Future& future) { future.Cancel(); }
        )
        {
            return;
        }
        for (auto& request : tasks_.AliveItems()) {
            try {
                if constexpr (requires(Future& future) { future.RequestCancel(); }) {
                    request.future.value().RequestCancel();
                } else if constexpr (requires(Future& future) { future.Cancel(); }) {
                    request.future.value().Cancel();
                }
                // Keep the future alive until WaitPending() drains its awaitable token.
            } catch (const std::exception&) {
                // Preserve the original error if cancellation itself fails.
            }
        }
    }

    void WaitPending() {
        // TODO: implement a WaitAll primitive and use it here to wait for cancelled tasks.
        // Wait for completion without retrieving results or rethrowing operation errors.
        while (wait_any_.GetSize() != 0) {
            [[maybe_unused]] const auto ready = WaitForCollectedTask(wait_any_);
        }
    }

    void CancelAndWait() noexcept {
        const engine::TaskCancellationBlocker cancellation_blocker;
        CancelPending();
        WaitPending();
    }

    const std::size_t concurrency_;
    const Start& start_;
    const CollectFailurePolicy failure_policy_;
    std::vector<std::optional<Outcome>> results_;
    utils::SlotMap<InFlight> tasks_;
    // Destroy the wait context before the futures whose tokens it references.
    engine::WaitAnyContext wait_any_;
};

}  // namespace impl

/// @ingroup userver_concurrency
///
/// @brief Collect results from lazily started operations with bounded concurrency.
///
/// Maintains a sliding window of in-flight operations: whenever an operation is collected,
/// its slot can be reused without waiting for earlier inputs. Calls @p start at most once
/// per input, without retries. Duplicate inputs are processed independently.
///
/// @param input_range Input range. Elements are forwarded to the factory as
/// `std::ranges::range_reference_t<Range>` without copying or storing them.
/// @param settings Concurrency limit and failure policy. See @ref concurrent::ParallelCollectSettings.
/// @param start Factory invoked with each range element, preserving its reference type and
/// value category, in the calling task. Returns an
/// owning @ref engine::Awaitable with a `Get()` or `get()` method, such as @ref engine::TaskWithResult,
/// @ref engine::Future, or a generated HTTP response future. If both methods exist, `Get()` is used.
/// The factory controls task creation, tracing, timeouts, and client retry settings.
///
/// @returns A vector of @ref utils::expected values in input order, each containing either
/// the operation's result or a `std::exception_ptr` preserving the exception's dynamic type.
/// Completion order does not affect result order. Supports void and move-only results.
/// For a reusable input range, callers can associate inputs and results by index or zip them.
///
/// @par Failure handling
/// With @ref concurrent::CollectFailurePolicy::kContinue, `std::exception`-based errors from the factory
/// and `Get()`/`get()` are collected and all inputs are processed.
/// With @ref concurrent::CollectFailurePolicy::kFailFast, the first observed `std::exception` stops new launches.
/// Pending operations receive `RequestCancel()` when available, otherwise `Cancel()` when
/// available. Futures remain alive until all waits on their tokens have finished.
/// Pending operations are awaited via @ref engine::WaitAnyContext without retrieving
/// their results. Task cancellation is blocked during cleanup. Errors from cancellation requests
/// derived from `std::exception` are ignored; the original exception is rethrown and no partial
/// results are returned. Future types do not need to provide `Wait()` or `wait()`.
///
/// @warning Fail-fast cleanup can still block while awaiting non-cancellable work or tasks
/// that do not respond to cancellation. Configure operation timeouts in @p start.
///
/// @par Cancellation and lifetimes
/// Parent cancellation and non-std exceptions propagate under either policy. During stack
/// unwinding, outstanding awaitables are destroyed according to their own RAII semantics.
/// Work started by the factory must manage its lifetime accordingly. When capturing input
/// references in asynchronous work, the caller must ensure that the referenced elements remain
/// valid until that work finishes. Single-pass and lazy ranges may invalidate references when
/// advanced; copy or move such inputs into the operation before the factory returns. A copied
/// non-owning input (such as `std::string_view`) does not extend the lifetime of its referent.
///
/// @throws `std::invalid_argument` If `settings.max_parallel_tasks` is zero, even for an empty range.
/// @throws `std::exception` The first observed operation error in fail-fast mode, with its
/// original dynamic type. Infrastructure errors may propagate under either policy.
///
/// @par Examples
/// Collecting move-only task results, including duplicate inputs:
/// @snippet core/src/concurrent/collect_in_parallel_test.cpp sample CollectInParallel tasks
/// Collecting individual startup and task errors with the default continue policy
/// (`LookupError` is the test's exception type):
/// @snippet core/src/concurrent/collect_in_parallel_test.cpp sample CollectInParallel continue
/// Stopping on a startup error and skipping the remaining inputs:
/// @snippet core/src/concurrent/collect_in_parallel_test.cpp sample CollectInParallel fail fast
template <std::ranges::input_range Range, typename Start>
requires std::invocable<const Start&, std::ranges::range_reference_t<Range>> &&
             impl::CollectableFuture<std::invoke_result_t<const Start&, std::ranges::range_reference_t<Range>>>
auto CollectInParallel(Range&& input_range, ParallelCollectSettings settings, const Start& start)
    -> std::vector<utils::expected<
        impl::CollectedValue<std::invoke_result_t<const Start&, std::ranges::range_reference_t<Range>>>,
        std::exception_ptr>> {
    const auto concurrency = *settings.max_parallel_tasks;
    if (concurrency == 0) {
        throw std::invalid_argument("Max parallel tasks must be greater than zero");
    }
    impl::ParallelCollector<std::ranges::range_reference_t<Range>, Start>
        collector{concurrency, start, settings.failure_policy};
    return collector.Run(std::forward<Range>(input_range));
}

}  // namespace concurrent

USERVER_NAMESPACE_END
