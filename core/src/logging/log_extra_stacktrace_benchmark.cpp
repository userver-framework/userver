#include <benchmark/benchmark.h>

#include <boost/stacktrace.hpp>
#include <functional>
#include <logging/log_extra.hpp>

void Stacktrace(benchmark::State& state) {
  std::function<logging::LogExtra(unsigned)> recursion;
  recursion = [&recursion](unsigned depth) {
    if (depth) {
      return recursion(depth - 1);
    } else {
      return logging::LogExtra::StacktraceNocache();
    }
  };

  const auto approximate_depth = state.range(0);
  for (auto _ : state) {
    auto result = recursion(approximate_depth);
    benchmark::DoNotOptimize(result);
  }

  // Detect true stack depth.
  std::function<boost::stacktrace::stacktrace(unsigned)> detect_depth;
  detect_depth = [&detect_depth](unsigned depth) {
    if (depth) {
      return detect_depth(depth - 1);
    } else {
      return boost::stacktrace::stacktrace{};
    }
  };
  state.counters["stack_depth"] = detect_depth(approximate_depth).size();
}

BENCHMARK(Stacktrace)->RangeMultiplier(4)->Range(1, 1024);

void StacktraceCached(benchmark::State& state) {
  std::function<logging::LogExtra(unsigned)> recursion;
  recursion = [&recursion](unsigned depth) {
    if (depth) {
      return recursion(depth - 1);
    } else {
      return logging::LogExtra::Stacktrace();
    }
  };

  const auto approximate_depth = state.range(0);
  for (auto _ : state) {
    auto result = recursion(approximate_depth);
    benchmark::DoNotOptimize(result);
  }

  // Detect true stack depth.
  std::function<boost::stacktrace::stacktrace(unsigned)> detect_depth;
  detect_depth = [&detect_depth](unsigned depth) {
    if (depth) {
      return detect_depth(depth - 1);
    } else {
      return boost::stacktrace::stacktrace{};
    }
  };
  state.counters["stack_depth"] = detect_depth(approximate_depth).size();
}

BENCHMARK(StacktraceCached)->RangeMultiplier(4)->Range(1, 1024);

void StacktraceGet(benchmark::State& state) {
  std::function<boost::stacktrace::stacktrace(unsigned)> recursion;
  recursion = [&recursion](unsigned depth) {
    if (depth) {
      return recursion(depth - 1);
    } else {
      return boost::stacktrace::stacktrace{};
    }
  };

  const auto approximate_depth = state.range(0);
  for (auto _ : state) {
    auto result = recursion(approximate_depth);
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(StacktraceGet)->RangeMultiplier(4)->Range(1, 1024);
