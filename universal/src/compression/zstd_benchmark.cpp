#include <benchmark/benchmark.h>

#include <chrono>
#include <random>

#include <zstd.h>
#include <userver/compression/zstd.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

std::string GenerateRandomData(std::size_t size) {
  std::mt19937 random_device(
      std::chrono::steady_clock::now().time_since_epoch().count());
  std::uniform_int_distribution dist(0, 25);

  std::string output;
  for (std::size_t ind = 0; ind < size; ++ind) {
    char rand_char = 'a' + static_cast<char>(dist(random_device));
    output.push_back(rand_char);
  }
  return output;
}

static void ZstdDecompress(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    state.PauseTiming();
    const auto kSize = state.range(0);
    auto data = GenerateRandomData(kSize);

    const auto kMaxSize = ZSTD_compressBound(kSize);
    std::string comp_buf(kMaxSize, '\0');

    const auto kCompSize =
        ZSTD_compress(comp_buf.data(), kMaxSize, data.data(), kSize, 1);

    if (ZSTD_isError(kCompSize)) {
      LOG_ERROR() << "Couldn't compress data!";
      return;
    }

    state.ResumeTiming();
    auto decompressed = compression::zstd::Decompress(
        std::string_view(comp_buf.data(), kCompSize), kSize);
  }
}
BENCHMARK(ZstdDecompress)->RangeMultiplier(2)->Range(1 << 10, 1 << 15);

USERVER_NAMESPACE_END
