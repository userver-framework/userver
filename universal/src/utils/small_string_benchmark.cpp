#include <userver/utils/small_string.hpp>

#include <array>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr size_t kArraySize = 1000;
}

std::string GenerateString(size_t size) {
  std::string_view chars{"0123456789"};
  std::string result;
  for (size_t i = 0; i < size; i++) result += chars[i % 10];
  return result;
}

static void SmallString_Std(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    std::string str{s};
    benchmark::DoNotOptimize(str);
  }
}
BENCHMARK(SmallString_Std)->Range(2, 2 << 10);

static void SmallString_Small(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    utils::SmallString<1000> str{s};
    benchmark::DoNotOptimize(str);
  }
}
BENCHMARK(SmallString_Small)->Range(2, 2 << 10);

static void SmallString_Std_Copy(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  std::array<std::string, kArraySize> str, str2;
  for (auto& x : str) x = s;
  for ([[maybe_unused]] auto _ : state) {
    for (size_t i = 0; i < str.size(); i++) str2[i] = str[i];
    state.PauseTiming();
    str2.fill({});
    state.ResumeTiming();
  }
}
BENCHMARK(SmallString_Std_Copy)
    ->Range(2, 2 << 10)
    ->Unit(benchmark::kMicrosecond);

static void SmallString_Small_Copy(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  std::array<utils::SmallString<1000>, kArraySize> str, str2;
  for (auto& x : str) x = s;
  for ([[maybe_unused]] auto _ : state) {
    for (size_t i = 0; i < str.size(); i++) str2[i] = str[i];
    state.PauseTiming();
    str2.fill({});
    state.ResumeTiming();
  }
}
BENCHMARK(SmallString_Small_Copy)
    ->Range(2, 2 << 10)
    ->Unit(benchmark::kMicrosecond);

static void SmallString_Std_Move(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  std::array<std::string, kArraySize> str, str2;
  for (auto& x : str) x = s;
  for ([[maybe_unused]] auto _ : state) {
    for (size_t i = 0; i < str.size(); i++) str2[i] = std::move(str[i]);
    state.PauseTiming();
    str2.fill({});
    state.ResumeTiming();
  }
}
BENCHMARK(SmallString_Std_Move)
    ->Range(2, 2 << 10)
    ->Unit(benchmark::kMicrosecond);

static void SmallString_Small_Move(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  std::array<utils::SmallString<1000>, kArraySize> str, str2;
  for (auto& x : str) x = s;
  for ([[maybe_unused]] auto _ : state) {
    for (size_t i = 0; i < str.size(); i++) str2[i] = std::move(str[i]);
    state.PauseTiming();
    str2.fill({});
    state.ResumeTiming();
  }
}
BENCHMARK(SmallString_Small_Move)
    ->Range(2, 2 << 10)
    ->Unit(benchmark::kMicrosecond);

USERVER_NAMESPACE_END
