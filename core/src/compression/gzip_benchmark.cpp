#include <benchmark/benchmark.h>

#include <chrono>
#include <random>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <compression/gzip.hpp>

USERVER_NAMESPACE_BEGIN

std::string GenerateRandomData(std::size_t size) {
    std::mt19937 random_device(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution dist(0, 25);

    std::string output;
    for (std::size_t ind = 0; ind < size; ++ind) {
        char rand_char = 'a' + static_cast<char>(dist(random_device));
        output.push_back(rand_char);
    }

    return output;
}

static void GzipDecompress(benchmark::State& state) {
    constexpr int kCompBufSize = 1024;

    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();
        const auto kSize = state.range(0);
        auto data = GenerateRandomData(kSize);

        namespace bio = boost::iostreams;

        bio::filtering_istream stream;
        stream.push(bio::gzip_compressor());
        stream.push(bio::array_source(data.data(), kSize));

        std::string compressed;

        while (stream) {
            char buf[kCompBufSize];
            stream.read(buf, sizeof(buf));
            compressed.insert(compressed.end(), buf, buf + stream.gcount());
        }

        state.ResumeTiming();

        auto decompressed = compression::gzip::Decompress(compressed, 1 << 30);
    }
}
BENCHMARK(GzipDecompress)->RangeMultiplier(2)->Range(1 << 10, 1 << 15);

USERVER_NAMESPACE_END
