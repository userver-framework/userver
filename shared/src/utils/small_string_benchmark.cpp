#include <userver/utils/small_string.hpp>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

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
  for ([[maybe_unused]] auto _ : state) {
    std::string str{s};
    std::string str2{str};
    benchmark::DoNotOptimize(str);
    benchmark::DoNotOptimize(str2);
  }
}
BENCHMARK(SmallString_Std_Copy)->Range(2, 2 << 10);

static void SmallString_Small_Copy(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    utils::SmallString<1000> str{s};
    utils::SmallString<1000> str2{str};
    benchmark::DoNotOptimize(str);
    benchmark::DoNotOptimize(str2);
  }
}
BENCHMARK(SmallString_Small_Copy)->Range(2, 2 << 10);

static void SmallString_Std_Move(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    std::string str{s};
    std::string str2{std::move(str)};
    benchmark::DoNotOptimize(str);
    benchmark::DoNotOptimize(str2);
  }
}
BENCHMARK(SmallString_Std_Move)->Range(2, 2 << 10);

static void SmallString_Small_Move(benchmark::State& state) {
  auto s = GenerateString(state.range(0));
  for ([[maybe_unused]] auto _ : state) {
    utils::SmallString<1000> str{s};
    utils::SmallString<1000> str2{std::move(str)};
    benchmark::DoNotOptimize(str);
    benchmark::DoNotOptimize(str2);
  }
}
BENCHMARK(SmallString_Small_Move)->Range(2, 2 << 10);

USERVER_NAMESPACE_END
