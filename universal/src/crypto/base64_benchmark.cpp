#include <benchmark/benchmark.h>

#include <cstddef>
#include <string>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/hash.hpp>

#include <crypto/impl/base64_cryptopp.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::string MakeBinaryData(std::size_t size) {
    std::string data(size, '\0');
    for (std::size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>(i * 131 + size * 7);
    }
    return data;
}

void Base64Encode(benchmark::State& state) {
    const auto data = MakeBinaryData(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::Base64Encode(data));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

void Base64EncodeCryptoPP(benchmark::State& state) {
    const auto data = MakeBinaryData(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::impl::CryptoPPBase64Encode(data, crypto::base64::Pad::kWith));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

void Base64Decode(benchmark::State& state) {
    const auto encoded = crypto::base64::Base64Encode(MakeBinaryData(state.range(0)));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::Base64Decode(encoded));
    }
    state.SetBytesProcessed(state.iterations() * encoded.size());
}

void Base64DecodeCryptoPP(benchmark::State& state) {
    const auto encoded = crypto::base64::Base64Encode(MakeBinaryData(state.range(0)));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::impl::CryptoPPBase64Decode(encoded));
    }
    state.SetBytesProcessed(state.iterations() * encoded.size());
}

// Unpadded input is padded in a copy before the SIMD decoder.
// None of the benchmarked sizes is a multiple of 3, so the encoded data always needs padding.
void Base64DecodeUnpadded(benchmark::State& state) {
    const auto encoded = crypto::base64::Base64Encode(MakeBinaryData(state.range(0)), crypto::base64::Pad::kWithout);
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::Base64Decode(encoded));
    }
    state.SetBytesProcessed(state.iterations() * encoded.size());
}

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
void Base64UrlEncode(benchmark::State& state) {
    const auto data = MakeBinaryData(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::Base64UrlEncode(data, crypto::base64::Pad::kWithout));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

void Base64UrlEncodeCryptoPP(benchmark::State& state) {
    const auto data = MakeBinaryData(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::impl::CryptoPPBase64UrlEncode(data, crypto::base64::Pad::kWithout));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

void Base64UrlDecode(benchmark::State& state) {
    const auto encoded = crypto::base64::Base64UrlEncode(MakeBinaryData(state.range(0)), crypto::base64::Pad::kWithout);
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::Base64UrlDecode(encoded));
    }
    state.SetBytesProcessed(state.iterations() * encoded.size());
}

void Base64UrlDecodeCryptoPP(benchmark::State& state) {
    const auto encoded = crypto::base64::Base64UrlEncode(MakeBinaryData(state.range(0)), crypto::base64::Pad::kWithout);
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::base64::impl::CryptoPPBase64UrlDecode(encoded));
    }
    state.SetBytesProcessed(state.iterations() * encoded.size());
}
#endif

// crypto::hash with OutputEncoding::kBase64, as used by S3 request signing
void HmacSha1Base64(benchmark::State& state) {
    const auto message = MakeBinaryData(state.range(0));
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(crypto::hash::HmacSha1("secret", message, crypto::hash::OutputEncoding::kBase64));
    }
}

}  // namespace

BENCHMARK(Base64Encode)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64EncodeCryptoPP)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64Decode)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64DecodeCryptoPP)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64DecodeUnpadded)->RangeMultiplier(8)->Range(8, 32768);
#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
BENCHMARK(Base64UrlEncode)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64UrlEncodeCryptoPP)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64UrlDecode)->RangeMultiplier(8)->Range(8, 32768);
BENCHMARK(Base64UrlDecodeCryptoPP)->RangeMultiplier(8)->Range(8, 32768);
#endif
BENCHMARK(HmacSha1Base64)->Arg(64)->Arg(1024);

USERVER_NAMESPACE_END
