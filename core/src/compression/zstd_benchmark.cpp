#include <benchmark/benchmark.h>

#include <iostream>
#include <random>

#include <zstd.h>
#include <compression/zstd.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <compression/gzip.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

std::string GenerateRandomData(std::size_t size) {
  std::mt19937 random_device;
  std::uniform_int_distribution<int> dist(0, 25);
  std::string output = "";
  for (std::size_t ind = 0; ind < size; ++ind) {
    char rand_char = 'a' + static_cast<char>(dist(random_device));
    output.push_back(rand_char);
  }
  return output;
}

static void ZstdDecompress(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    state.PauseTiming();
    const std::size_t kSize = state.range(0);
    auto data = GenerateRandomData(kSize);

    const std::size_t kMaxSize = ZSTD_compressBound(kSize);
    char* comp_buf = new char[kMaxSize];

    const std::size_t kCompSize =
        ZSTD_compress(comp_buf, kMaxSize, data.data(), kSize, 1);

    if (ZSTD_isError(kCompSize)) {
      LOG_ERROR() << "Couldn't compress data!";
      return;
    }

    state.ResumeTiming();
    auto decomp_str = compression::zstd::Decompress(
        std::string_view(comp_buf, kCompSize), kSize);
  }
}
BENCHMARK(ZstdDecompress)->RangeMultiplier(2)->Range(1 << 10, 1 << 15);

static void GzipDecompress(benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    state.PauseTiming();
    const std::size_t kSize = state.range(0);
    auto data = GenerateRandomData(kSize);

    namespace bio = boost::iostreams;

    constexpr int kCompBufSize = 1024;

    bio::filtering_istream stream;
    stream.push(bio::gzip_compressor());
    stream.push(bio::array_source(data.data(), kSize));

    std::string compressed = "";

    while (stream) {
      char buf[kCompBufSize];
      stream.read(buf, sizeof(buf));
      compressed.insert(compressed.end(), buf, buf + stream.gcount());
    }

    state.ResumeTiming();
    auto decomp_str = compression::gzip::Decompress(
        compressed, 1 << 30);
  }
}
BENCHMARK(GzipDecompress)->RangeMultiplier(2)->Range(1 << 10, 1 << 15);


USERVER_NAMESPACE_END
