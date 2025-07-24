#include <benchmark/benchmark.h>

#include <userver/utils/log.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

#include <tests/logging.pb.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

sample::ugrpc::LoggingMessage ConstructMessage() {
    sample::ugrpc::LoggingMessage message;

    message.set_id("test-id");

    for (int i = 0; i < 10; ++i) {
        *message.add_names() = "test-name-" + std::to_string(i);
    }

    for (int i = 0; i < 100; ++i) {
        auto* item = message.add_items();
        item->set_index(i);
        item->set_value("test-value-" + std::to_string(i));
    }

    for (int i = 0; i < 1000; ++i) {
        (*message.mutable_properties())["test-property-name-" + std::to_string(i)] =
            "test-property-" + std::to_string(i);
    }

    return message;
}
const auto kMessage = ConstructMessage();

}  // namespace

void BenchDebugStringAndLimit(benchmark::State& state) {
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    for (auto _ : state) {
        {
            auto log = utils::log::ToLimitedUtf8(kMessage.Utf8DebugString(), 1024);
            benchmark::DoNotOptimize(log);
        }
    }
}
BENCHMARK(BenchDebugStringAndLimit);

void BenchCustomLimit(benchmark::State& state) {
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    for (auto _ : state) {
        {
            auto log = impl::ToLimitedDebugString(kMessage, 1024);
            benchmark::DoNotOptimize(log);
        }
    }
}
BENCHMARK(BenchCustomLimit);

}  // namespace ugrpc

USERVER_NAMESPACE_END
