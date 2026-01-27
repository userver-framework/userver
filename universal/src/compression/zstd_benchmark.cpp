#include <benchmark/benchmark.h>

#include <chrono>
#include <random>

#include <zstd.h>
#include <userver/compression/zstd.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

std::string GenerateRandomData(std::size_t size) {
    std::mt19937 random_device(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution dist(0, 25);

    std::string output;
    for (std::size_t ind = 0; ind < size; ++ind) {
        const char rand_char = 'a' + static_cast<char>(dist(random_device));
        output.push_back(rand_char);
    }
    return output;
}

static void ZstdDecompress(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();
        const auto size = state.range(0);
        const auto data = GenerateRandomData(size);

        const auto max_size = ZSTD_compressBound(size);
        std::string comp_buf(max_size, '\0');

        const auto comp_size = ZSTD_compress(comp_buf.data(), max_size, data.data(), size, 1);

        if (ZSTD_isError(comp_size)) {
            LOG_ERROR() << "Couldn't compress data!";
            return;
        }

        state.ResumeTiming();
        auto decompressed = compression::zstd::Decompress(std::string_view(comp_buf.data(), comp_size), size);
    }
}
BENCHMARK(ZstdDecompress)->RangeMultiplier(2)->Range(1 << 10, 1 << 15);

USERVER_NAMESPACE_END
