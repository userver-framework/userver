#include <benchmark/benchmark.h>

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <spdlog/sinks/null_sink.h>

#include <logging/log.hpp>

#include <utils/gbench_auxilary.hpp>

class LogHelperBenchmark : public benchmark::Fixture {
 public:
  LogHelperBenchmark() { old_ = logging::SetDefaultLogger(GetNullLogger()); }
  ~LogHelperBenchmark() {
    if (old_) logging::SetDefaultLogger(old_);
  }

 private:
  static logging::LoggerPtr GetNullLogger() {
    static auto null_logger =
        spdlog::create<spdlog::sinks::null_sink_st>("null_logger");
    return null_logger;
  }
  logging::LoggerPtr old_;
};

BENCHMARK_DEFINE_TEMPLATE_F(LogHelperBenchmark, LogNumber)
(benchmark::State& state) {
  T msg{42};
  for (auto _ : state) {
    LOG_INFO() << msg;
  }
}

BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, int);
BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, long);
BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, float);
BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, double);

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogString)(benchmark::State& state) {
  std::string msg(state.range(0), '*');
  for (auto _ : state) {
    LOG_INFO() << msg;
  }
  state.SetComplexityN(state.range(0));
}
// Run benchmarks to output string of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogString)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogChar)(benchmark::State& state) {
  std::string msg(state.range(0), '*');
  for (auto _ : state) {
    LOG_INFO() << msg.c_str();
  }
  state.SetComplexityN(state.range(0));
}
// Run benchmarks to output char arrays of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogChar)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogCheck)(benchmark::State& state) {
  std::string msg(state.range(0), '*');
  for (auto _ : state) {
    LOG_TRACE() << msg.c_str();
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogCheck)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();

struct StreamedStruct {
  int64_t intVal;
  std::string stringVal;
};

std::ostream& operator<<(std::ostream& os, const StreamedStruct& value) {
  std::ostream::sentry s(os);
  if (s) {
    os << value.intVal << " " << value.stringVal;
  }
  return os;
}

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogStruct)(benchmark::State& state) {
  StreamedStruct msg{state.range(0), std::string(state.range(0), '*')};
  for (auto _ : state) {
    LOG_INFO() << msg;
  }
  state.SetComplexityN(state.range(0));
}
// Run benchmarks to structs with strings of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogStruct)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();
