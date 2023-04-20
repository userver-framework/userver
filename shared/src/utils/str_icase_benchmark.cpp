#include <benchmark/benchmark.h>

#include <vector>

#include <userver/utils/rand.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string GenerateRandomString(std::size_t len) {
  std::string result(len, 'a');
  for (auto& c : result) {
    const auto val = utils::RandRange(52);
    c = val > 25 ? ('A' + val - 26) : ('a' + val);
  }

  return result;
}

std::string GenerateRandomLowercaseString(std::size_t len) {
  std::string result(len, 'a');
  for (auto& c : result) {
    c += utils::RandRange(26);
  }

  return result;
}

}  // namespace

template <typename Hasher>
void HashLowercaseString(benchmark::State& state) {
  const Hasher hasher{};

  const auto data = GenerateRandomLowercaseString(state.range(0));

  for (auto _ : state) {
    for (std::size_t i = 0; i < 20; ++i) {
      benchmark::DoNotOptimize(hasher(data));
    }
  }
}

BENCHMARK_TEMPLATE(HashLowercaseString, std::hash<std::string>)
    ->DenseRange(8, 64, 4);
BENCHMARK_TEMPLATE(HashLowercaseString, utils::StrCaseHash)
    ->DenseRange(8, 64, 2);
BENCHMARK_TEMPLATE(HashLowercaseString, utils::StrIcaseHash)
    ->DenseRange(8, 40, 1);

template <typename Hasher>
void HashRandomCaseString(benchmark::State& state) {
  std::vector<std::string> random_strings;
  random_strings.reserve(1024);

  for (std::size_t i = 0; i < 1024; ++i) {
    random_strings.push_back(GenerateRandomString(state.range(0)));
  }

  std::size_t offset = 0;
  std::size_t index = 0;

  const Hasher hasher{};
  for (auto _ : state) {
    // fighting branch predictor to the best of my abilities here
    offset += index == 1024;
    index &= 1023;

    const auto real_index = (offset + index) & 1023;
    ++index;

    for (std::size_t i = 0; i < 20; ++i) {
      benchmark::DoNotOptimize(hasher(random_strings[real_index]));
    }
  }
}

BENCHMARK_TEMPLATE(HashRandomCaseString, utils::StrIcaseHash)
    ->DenseRange(8, 65, 3);

void CaseInsensitiveCompareEqualStrings(benchmark::State& state) {
  const auto len = state.range(0);

  const auto first = GenerateRandomString(len);
  const auto second = std::string{first};
  const auto cmp = utils::StrIcaseEqual{};

  for (auto _ : state) {
    for (std::size_t i = 0; i < 20; ++i) {
      benchmark::DoNotOptimize(cmp(first, second));
    }
  }
}

template <std::size_t StrLen>
void CaseInsensitiveCompareDifferentStrings(benchmark::State& state) {
  const auto diff_at = state.range(0) - 1;
  if (static_cast<std::size_t>(diff_at) >= StrLen) {
    state.SkipWithError("diff_at is bigger than string length");
    return;
  }

  auto first = GenerateRandomString(StrLen);
  first[diff_at] = 'a';
  std::string second{first};
  second[diff_at] = 'b';

  const auto cmp = utils::StrIcaseEqual{};

  for (auto _ : state) {
    for (std::size_t i = 0; i < 20; ++i) {
      benchmark::DoNotOptimize(cmp(first, second));
    }
  }
}

BENCHMARK(CaseInsensitiveCompareEqualStrings)->DenseRange(1, 31, 3);
BENCHMARK_TEMPLATE(CaseInsensitiveCompareDifferentStrings, 31)
    ->DenseRange(1, 31, 3);
BENCHMARK_TEMPLATE(CaseInsensitiveCompareDifferentStrings, 15)
    ->DenseRange(1, 15, 2);
BENCHMARK_TEMPLATE(CaseInsensitiveCompareDifferentStrings, 7)
    ->DenseRange(1, 7, 1);

USERVER_NAMESPACE_END
