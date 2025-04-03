#include <benchmark/benchmark.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/rand.hpp>

#include <engine/coro/stack_usage_monitor.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class StackConsumingScope final {
public:
    StackConsumingScope() {
        utils::WithDefaultRandom([this](auto& rnd) {
            for (auto& value : values_) {
                value = rnd();
            }
        });
        std::sort(values_.begin(), values_.end());
    }

    std::uint32_t Get() const noexcept { return values_[utils::RandRange(kValuesCount)]; }

private:
    static constexpr std::size_t kValuesCount = 256;
    std::array<std::uint32_t, kValuesCount> values_{};
};

__attribute__((noinline)) std::uint64_t SelfLimitingRecursiveFunction() {
    const StackConsumingScope scope{};

    const auto stack_size = engine::current_task::GetStackSize();
    const auto stack_usage = engine::coro::GetCurrentTaskStackUsageBytes();
    if (stack_usage >= stack_size * 0.85) {
        return 1;
    }

    return scope.Get() + SelfLimitingRecursiveFunction();
}

void create_async(benchmark::State& state, bool enable) {
    engine::TaskProcessorPoolsConfig config;
    config.is_stack_usage_monitor_enabled = enable;

    for ([[maybe_unused]] auto _ : state) {
        engine::RunStandalone(1, config, [&] { SelfLimitingRecursiveFunction(); });
    }
}

}  // namespace

void stack_usage_monitor_on(benchmark::State& state) { create_async(state, true); }
BENCHMARK(stack_usage_monitor_on);

void stack_usage_monitor_off(benchmark::State& state) { create_async(state, false); }
BENCHMARK(stack_usage_monitor_off);

USERVER_NAMESPACE_END
