#include <benchmark/benchmark.h>

#include <string>
#include <string_view>

#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string Repeat(std::string_view unit, std::size_t times) {
    std::string result;
    result.reserve(unit.size() * times);
    for (std::size_t i = 0; i < times; ++i) {
        result.append(unit);
    }
    return result;
}

void CountCodePoints(benchmark::State& state, const std::string& text) {
    for ([[maybe_unused]] auto _ : state) {
        auto count = utils::text::utf8::GetCodePointsCount(text);
        benchmark::DoNotOptimize(count);
    }
}

void CheckAscii(benchmark::State& state, const std::string& text) {
    for ([[maybe_unused]] auto _ : state) {
        auto ascii = utils::text::IsAscii(text);
        benchmark::DoNotOptimize(ascii);
    }
}

void CountAsciiString(benchmark::State& state) { CountCodePoints(state, std::string(state.range(0), 'a')); }
BENCHMARK(CountAsciiString)->Arg(7)->Arg(8)->Arg(15)->Arg(16)->Arg(31)->Arg(32)->Arg(4096);

void CheckAsciiString(benchmark::State& state) { CheckAscii(state, std::string(state.range(0), 'a')); }
BENCHMARK(CheckAsciiString)->Arg(7)->Arg(8)->Arg(15)->Arg(16)->Arg(31)->Arg(32)->Arg(4096);

BENCHMARK_CAPTURE(CountCodePoints, Cyrillic, Repeat("я", 2048));
BENCHMARK_CAPTURE(CountCodePoints, AsciiAfterCyrillic, "city=Москва" + std::string(4080, 'a'));
BENCHMARK_CAPTURE(CountCodePoints, Alternating, Repeat("aя", 1365));
BENCHMARK_CAPTURE(CheckAscii, NonAsciiAtStart, "Москва" + std::string(4084, 'a'));
BENCHMARK_CAPTURE(CheckAscii, NonAsciiAtEnd, std::string(4094, 'a') + "я");

}  // namespace

USERVER_NAMESPACE_END
