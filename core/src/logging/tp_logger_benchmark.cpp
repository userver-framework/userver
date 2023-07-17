#include <logging/tp_logger.hpp>

#include <benchmark/benchmark.h>

#include <logging/impl/null_sink.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::shared_ptr<logging::impl::TpLogger> MakeLoggerFromSink(
    const std::string& logger_name, logging::impl::SinkPtr sink_ptr,
    logging::Format format) {
  auto logger = std::make_shared<logging::impl::TpLogger>(format, logger_name);
  logger->AddSink(std::move(sink_ptr));
  return logger;
}

class TpLoggerBenchmark : public benchmark::Fixture {
 protected:
  void SetUp(const benchmark::State&) override {
    tp_logger_ =
        MakeLoggerFromSink("test", std::make_shared<logging::impl::NullSink>(),
                           logging::Format::kTskv);
    tp_logger_->SetLevel(logging::Level::kInfo);
    guard_.emplace(tp_logger_);
  }

  void TearDown(const benchmark::State&) override { guard_.reset(); }

  auto StartAsyncLoggerScope() {
    tp_logger_->StartConsumerTask(engine::current_task::GetTaskProcessor(),
                                  1 << 30,
                                  logging::QueueOverflowBehavior::kDiscard);
    return utils::FastScopeGuard(
        [this]() noexcept { tp_logger_->StopConsumerTask(); });
  }

 private:
  std::shared_ptr<logging::impl::TpLogger> tp_logger_;
  std::optional<logging::DefaultLoggerGuard> guard_;
};

}  // namespace

BENCHMARK_DEFINE_F(TpLoggerBenchmark, LogString)(benchmark::State& state) {
  engine::RunStandalone(2, [&] {
    auto scope = StartAsyncLoggerScope();
    const auto msg = Launder(std::string(state.range(0), '*'));
    for (auto _ : state) {
      LOG_INFO() << msg;
    }
    state.SetComplexityN(state.range(0));
  });
}
// Run benchmarks to output string of sizes of 8 bytes to 8 kilobytes
BENCHMARK_REGISTER_F(TpLoggerBenchmark, LogString)
    ->RangeMultiplier(2)
    ->Range(8, 8 << 10)
    ->Complexity();

USERVER_NAMESPACE_END
