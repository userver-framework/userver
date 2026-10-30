#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/encoding/tskv.hpp>

#include <logging/impl/formatters/tskv.hpp>

USERVER_NAMESPACE_BEGIN

void TskvFormatterConstructor(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        logging::impl::formatters::Tskv
            formatter(logging::Level::kDebug, logging::Format::kTskv, utils::impl::SourceLocation::Current());
        benchmark::DoNotOptimize(&formatter);
    }
}
BENCHMARK(TskvFormatterConstructor);

void TskvFormatterAndTags(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        logging::impl::formatters::Tskv
            formatter(logging::Level::kDebug, logging::Format::kTskv, utils::impl::SourceLocation::Current());
        formatter.AddTag("key1", std::string_view{"some_value"});
        formatter.AddTag("key2", std::string_view{"some value2"});
        formatter.AddTag("key 3", std::string_view{"some value2"});
        benchmark::DoNotOptimize(&formatter);
    }
}
BENCHMARK(TskvFormatterAndTags);

namespace {

namespace tskv = utils::encoding::impl::tskv;

template <typename Encoder>
void TskvValueEncode(benchmark::State& state) {
    constexpr std::array<std::string_view, 3> kPatterns{"abcdefgh", "Я🙂", "Я\n"};
    constexpr std::array<std::string_view, 3> kNames{"ascii", "utf8", "escaped-utf8"};
    const auto kind = static_cast<std::size_t>(state.range(0));
    alignas(32) std::array<char, 512> input;
    input.fill('a');
    const auto pattern = kPatterns[kind];
    for (std::size_t offset = 0; offset + pattern.size() <= input.size(); offset += pattern.size()) {
        std::ranges::copy(pattern, input.data() + offset);
    }
    const std::string_view view(input.data(), input.size());
    logging::impl::TextLogItem item;
    auto& output = item.log_line;
    output.reserve(tskv::MaxEncodedSize(input.size()) + tskv::PaddingSize<Encoder>());
    for ([[maybe_unused]] auto _ : state) {
        output.clear();
        tskv::EncodeFullyBuffered<Encoder>(output, view, utils::encoding::EncodeTskvMode::kValue);
        benchmark::DoNotOptimize(output.data());
        benchmark::ClobberMemory();
    }
    state.SetLabel(std::string(kNames[kind]));
    state.SetBytesProcessed(state.iterations() * input.size());
}

using tskv::EncoderStd;
BENCHMARK_TEMPLATE(TskvValueEncode, EncoderStd)->DenseRange(0, 2)->ArgName("input");

#ifdef __SSE2__
using tskv::EncoderSse2;
BENCHMARK_TEMPLATE(TskvValueEncode, EncoderSse2)->DenseRange(0, 2)->ArgName("input");
#endif

#ifdef __AVX2__
using tskv::EncoderAvx2;
BENCHMARK_TEMPLATE(TskvValueEncode, EncoderAvx2)->DenseRange(0, 2)->ArgName("input");
#endif

}  // namespace

USERVER_NAMESPACE_END
