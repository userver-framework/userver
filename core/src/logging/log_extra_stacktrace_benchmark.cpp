#include <userver/logging/log_extra.hpp>

#include <functional>
#include <optional>

#include <benchmark/benchmark.h>
#include <boost/stacktrace.hpp>

#include <userver/engine/run_standalone.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/null_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Stacktrace : public ::benchmark::Fixture {
protected:
    void SetUp(benchmark::State&) override {
        logger_scope_.emplace(logging::impl::MakeNoopLoggerForTests());
        logging::SetDefaultLoggerLevel(logging::Level::kDebug);
    }

    void TearDown(benchmark::State&) override { logger_scope_.reset(); }

private:
    std::optional<logging::DefaultLoggerGuard> logger_scope_;
};

}  // namespace

BENCHMARK_DEFINE_F(Stacktrace, NotCached)(benchmark::State& state) {
    std::function<logging::LogExtra(std::size_t)> recursion;
    recursion = [&recursion](std::size_t depth) {
        if (depth) {
            return recursion(depth - 1);
        } else {
            return logging::LogExtra::StacktraceNocache();
        }
    };

    const auto approximate_depth = state.range(0);
    for ([[maybe_unused]] auto _ : state) {
        auto result = recursion(approximate_depth);
        benchmark::DoNotOptimize(result);
    }

    // Detect true stack depth.
    std::function<boost::stacktrace::stacktrace(std::size_t)> detect_depth;
    detect_depth = [&detect_depth](std::size_t depth) {
        if (depth) {
            return detect_depth(depth - 1);
        } else {
            return boost::stacktrace::stacktrace{};
        }
    };
    state.counters["stack_depth"] = detect_depth(approximate_depth).size();
}

BENCHMARK_REGISTER_F(Stacktrace, NotCached)->RangeMultiplier(4)->Range(1, 1024);

BENCHMARK_DEFINE_F(Stacktrace, Cached)(benchmark::State& state) {
    std::function<logging::LogExtra(std::size_t)> recursion;
    recursion = [&recursion](std::size_t depth) {
        if (depth) {
            return recursion(depth - 1);
        } else {
            return logging::LogExtra::Stacktrace();
        }
    };

    const auto approximate_depth = state.range(0);
    for ([[maybe_unused]] auto _ : state) {
        auto result = recursion(approximate_depth);
        benchmark::DoNotOptimize(result);
    }

    // Detect true stack depth.
    std::function<boost::stacktrace::stacktrace(std::size_t)> detect_depth;
    detect_depth = [&detect_depth](std::size_t depth) {
        if (depth) {
            return detect_depth(depth - 1);
        } else {
            return boost::stacktrace::stacktrace{};
        }
    };
    state.counters["stack_depth"] = detect_depth(approximate_depth).size();
}

BENCHMARK_REGISTER_F(Stacktrace, Cached)->RangeMultiplier(4)->Range(1, 1024);

void StacktraceGetCommon(benchmark::State& state) {
    std::function<boost::stacktrace::stacktrace(std::size_t)> recursion;
    recursion = [&recursion](std::size_t depth) {
        if (depth) {
            return recursion(depth - 1);
        } else {
            return boost::stacktrace::stacktrace{};
        }
    };

    const auto approximate_depth = state.range(0);
    for ([[maybe_unused]] auto _ : state) {
        auto result = recursion(approximate_depth);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_DEFINE_F(Stacktrace, Get)(benchmark::State& state) { StacktraceGetCommon(state); }

BENCHMARK_REGISTER_F(Stacktrace, Get)->RangeMultiplier(4)->Range(1, 1024);

BENCHMARK_DEFINE_F(Stacktrace, GetInCoroutine)(benchmark::State& state) {
    engine::TaskProcessorPoolsConfig config{};
    config.coro_stack_size = 1024 * 1024;
    engine::RunStandalone(1, config, [&state]() { StacktraceGetCommon(state); });
}

BENCHMARK_REGISTER_F(Stacktrace, GetInCoroutine)->RangeMultiplier(4)->Range(1, 1024);

USERVER_NAMESPACE_END
