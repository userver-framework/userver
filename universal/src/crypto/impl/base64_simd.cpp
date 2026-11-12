#include <crypto/impl/base64_simd.hpp>

#include <cstddef>
#include <cstdint>

#ifndef USERVER_NO_SIMD_BASE64
#ifdef ARCADIA_ROOT
#include <contrib/libs/base64/avx2/libbase64.h>
#include <contrib/libs/base64/neon32/libbase64.h>
#include <contrib/libs/base64/neon64/libbase64.h>
#include <contrib/libs/base64/plain32/libbase64.h>
#include <contrib/libs/base64/plain64/libbase64.h>
#include <contrib/libs/base64/ssse3/libbase64.h>
#else
#include <libbase64.h>  // Y_IGNORE
#endif
#endif

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto::base64::impl {

#ifndef USERVER_NO_SIMD_BASE64

namespace {

#ifdef ARCADIA_ROOT
// Arcadia ships aklomp/base64 0.3.0 split into per-codec libraries, so the codec is chosen here
struct SimdCodec {
    void (*encode)(const char* src, std::size_t srclen, char* out, std::size_t* outlen);
    int (*decode)(const char* src, std::size_t srclen, char* out, std::size_t* outlen);
};

SimdCodec ChooseSimdCodec() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    if (__builtin_cpu_supports("avx2")) {
        return {avx2_base64_encode, avx2_base64_decode};
    }
    if (__builtin_cpu_supports("ssse3")) {
        return {ssse3_base64_encode, ssse3_base64_decode};
    }
#endif
#if defined(__aarch64__)
    return {neon64_base64_encode, neon64_base64_decode};
#elif defined(__arm__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
    // Without NEON the neon32 codec calls abort()
    return {neon32_base64_encode, neon32_base64_decode};
#elif UINTPTR_MAX == UINT64_MAX
    return {plain64_base64_encode, plain64_base64_decode};
#else
    return {plain32_base64_encode, plain32_base64_decode};
#endif
}

const SimdCodec& GetSimdCodec() noexcept {
    static const SimdCodec codec = ChooseSimdCodec();
    return codec;
}

void SimdEncode(std::string_view data, char* out, std::size_t& out_size) noexcept {
    GetSimdCodec().encode(data.data(), data.size(), out, &out_size);
}

bool SimdDecode(std::string_view data, char* out, std::size_t& out_size) noexcept {
    // Unlike upstream and crypto++, 0.3.0 also accepts the URL alphabet
    if (data.find('-') != std::string_view::npos || data.find('_') != std::string_view::npos) {
        return false;
    }
    return GetSimdCodec().decode(data.data(), data.size(), out, &out_size) == 1;
}
#else
// aklomp/base64 chooses the codec on the first call and caches it in a global without synchronization.
// Making the first call under a function-local static removes the data race.
void EnsureSimdCodecChosen() noexcept {
    static const bool kChosen = [] {
        char out[4];
        std::size_t out_size = 0;
        base64_encode("", 0, out, &out_size, 0);
        return true;
    }();
    (void)kChosen;
}

void SimdEncode(std::string_view data, char* out, std::size_t& out_size) noexcept {
    EnsureSimdCodecChosen();
    base64_encode(data.data(), data.size(), out, &out_size, 0);
}

bool SimdDecode(std::string_view data, char* out, std::size_t& out_size) noexcept {
    EnsureSimdCodecChosen();
    return base64_decode(data.data(), data.size(), out, &out_size, 0) == 1;
}
#endif

// Replaces From62 with To62 and From63 with To63. Written with masks instead of branches, so that the
// compiler vectorizes it.
template <char From62, char From63, char To62, char To63>
void TranslateAlphabet(std::string& data) noexcept {
    for (auto& c : data) {
        const auto byte = static_cast<unsigned char>(c);
        const unsigned char mask62 = -static_cast<unsigned char>(byte == static_cast<unsigned char>(From62));
        const unsigned char mask63 = -static_cast<unsigned char>(byte == static_cast<unsigned char>(From63));
        c = static_cast<char>(byte ^ (mask62 & (From62 ^ To62)) ^ (mask63 & (From63 ^ To63)));
    }
}

}  // namespace

std::string SimdBase64Encode(std::string_view data, Pad pad, Alphabet alphabet) {
    std::string result((data.size() + 2) / 3 * 4, '\0');
    std::size_t out_size = 0;
    SimdEncode(data, result.data(), out_size);
    if (pad == Pad::kWithout) {
        while (out_size > 0 && result[out_size - 1] == '=') {
            --out_size;
        }
    }
    result.resize(out_size);
    if (alphabet == Alphabet::kUrl) {
        TranslateAlphabet<'+', '/', '-', '_'>(result);
    }
    return result;
}

std::optional<std::string> SimdBase64Decode(std::string_view data, Alphabet alphabet) {
    if (data.size() % 4 == 1) {
        return std::nullopt;
    }

    std::string buffer;
    std::string_view input = data;
    if (alphabet == Alphabet::kUrl || data.size() % 4 != 0) {
        if (alphabet == Alphabet::kUrl &&
            (data.find('+') != std::string_view::npos || data.find('/') != std::string_view::npos))
        {
            return std::nullopt;
        }
        buffer.reserve((data.size() + 3) / 4 * 4);
        buffer.assign(data);
        if (alphabet == Alphabet::kUrl) {
            TranslateAlphabet<'-', '_', '+', '/'>(buffer);
        }
        buffer.resize((data.size() + 3) / 4 * 4, '=');
        input = buffer;
    }
    UASSERT_MSG(input.size() % 4 == 0, "Input must be padded to whole 4-character groups");

    std::size_t padding = 0;
    while (padding < input.size() && input[input.size() - 1 - padding] == '=') {
        ++padding;
    }
    if (padding > 2) {
        return std::nullopt;
    }

    std::string result(input.size() / 4 * 3, '\0');
    std::size_t out_size = 0;
    // A decoder may stop at a padding character in the middle (0.3.0 also stops at ',') and still report
    // success, so the output length is checked as well
    if (!SimdDecode(input, result.data(), out_size) || out_size != result.size() - padding) {
        return std::nullopt;
    }
    result.resize(out_size);
    return result;
}

#endif

}  // namespace crypto::base64::impl

USERVER_NAMESPACE_END
