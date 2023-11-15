#include <benchmark/benchmark.h>

#include <ostream>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class NoopLogger : public logging::impl::LoggerBase {
 public:
  NoopLogger() noexcept : LoggerBase(logging::Format::kRaw) {
    SetLevel(logging::Level::kInfo);
  }
  void Log(logging::Level, std::string_view) override {}
  void Flush() override {}
};

class PrependedTagLogger final : public NoopLogger {
 public:
  void PrependCommonTags(logging::impl::TagWriter writer) const override {
    writer.PutTag("aaaaaaaaaaaaaaaaaa", "value");
    writer.PutTag("bbbbbbbbbb", 42);
    writer.PutTag("ccccccccccccccccccccccc", 42.0);
    writer.PutTag("dddddddddddddddd", std::chrono::milliseconds{42});
    writer.PutTag("eeeeeeeee", true);
    writer.PutTag("ffffffffffffffffffffff", "foo");
    writer.PutTag("gggggggggggggggggggg", "bar");
    writer.PutTag("hhhhhhhhhhhhhh", "baz");
    writer.PutTag("iiiiiiiiiii", "qux");
    writer.PutTag("jjjjjjjjjjjjjjjjjj", "quux");
  }
};

class LogHelperBenchmark : public benchmark::Fixture {
  void SetUp(const benchmark::State&) override {
    guard_.emplace(std::make_shared<NoopLogger>());
  }

  void TearDown(const benchmark::State&) override { guard_.reset(); }

  std::optional<logging::DefaultLoggerGuard> guard_;
};

BENCHMARK_DEFINE_TEMPLATE_F(LogHelperBenchmark, LogNumber)
(benchmark::State& state) {
  const auto msg = Launder(T{42});
  for ([[maybe_unused]] auto _ : state) {
    LOG_INFO() << msg;
  }
}

BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, int);
BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, long);
BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, float);
BENCHMARK_INSTANTIATE_TEMPLATE_F(LogHelperBenchmark, LogNumber, double);

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogString)(benchmark::State& state) {
  const auto msg = Launder(std::string(state.range(0), '*'));
  for ([[maybe_unused]] auto _ : state) {
    LOG_INFO() << msg;
  }
  state.SetComplexityN(state.range(0));
}
// Run benchmarks to output string of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogString)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Arg(768)  // Just above initial_capacity/2
    ->Complexity();

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogChar)(benchmark::State& state) {
  const auto msg = Launder(std::string(state.range(0), '*'));
  for ([[maybe_unused]] auto _ : state) {
    LOG_INFO() << msg.c_str();
  }
  state.SetComplexityN(state.range(0));
}
// Run benchmarks to output char arrays of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogChar)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();

__attribute__((noinline)) void LogTrace() { LOG_TRACE() << 42; }

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogCheck)(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    LogTrace();
  }
}
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogCheck);

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
  const StreamedStruct msg{state.range(0),
                           Launder(std::string(state.range(0), '*'))};
  for ([[maybe_unused]] auto _ : state) {
    LOG_INFO() << msg;
  }
  state.SetComplexityN(state.range(0));
}
// Run benchmarks to structs with strings of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogStruct)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();

struct MultipleStrings final {
  std::string str;
  std::size_t count{};
};

logging::LogHelper& operator<<(logging::LogHelper& lh,
                               const MultipleStrings& strings) {
  for (std::size_t i = 0; i < strings.count; ++i) {
    lh << strings.str;
  }
  return lh;
}

BENCHMARK_DEFINE_F(LogHelperBenchmark, LogMultipleStrings)
(benchmark::State& state) {
  const MultipleStrings msg{Launder(std::string(state.range(0), '*')),
                            static_cast<std::size_t>(state.range(1))};
  for ([[maybe_unused]] auto _ : state) {
    LOG_INFO() << msg;
  }
}
BENCHMARK_REGISTER_F(LogHelperBenchmark, LogMultipleStrings)
    ->Args({0, 0})
    ->Args({1, 1})
    ->Args({2, 2})
    ->RangeMultiplier(2)
    ->Ranges({{4, 32}, {4, 32}});

void LogPrependedTags(benchmark::State& state) {
  const logging::DefaultLoggerGuard guard{
      std::make_shared<PrependedTagLogger>()};

  for ([[maybe_unused]] auto _ : state) {
    LOG_INFO() << "";
  }
}
BENCHMARK(LogPrependedTags);

}  // namespace

USERVER_NAMESPACE_END
