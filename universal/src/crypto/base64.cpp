#include <userver/crypto/base64.hpp>

#include <cstdint>
#include <optional>
#include <utility>

#include <cryptopp/base64.h>

#include <userver/crypto/exception.hpp>

#include <crypto/impl/base64_cryptopp.hpp>
#include <crypto/impl/base64_simd.hpp>

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

USERVER_NAMESPACE_BEGIN

namespace crypto::base64 {

namespace {

// Alphabet for universal Base64 conversion (both traditional and "web-safe" character are included)
// Copied from protobuf 22.5 .
constexpr std::int8_t kBase64UniversalTable[256] = {
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       62 /*+*/,
    -1,       62 /*-*/, -1,       63 /*/ */, 52 /*0*/, 53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/, 57 /*5*/, 58 /*6*/,
    59 /*7*/, 60 /*8*/, 61 /*9*/, -1,        -1,       -1,       -1,       -1,       -1,       -1,       0 /*A*/,
    1 /*B*/,  2 /*C*/,  3 /*D*/,  4 /*E*/,   5 /*F*/,  6 /*G*/,  07 /*H*/, 8 /*I*/,  9 /*J*/,  10 /*K*/, 11 /*L*/,
    12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/,  16 /*Q*/, 17 /*R*/, 18 /*S*/, 19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/,
    23 /*X*/, 24 /*Y*/, 25 /*Z*/, -1,        -1,       -1,       -1,       63 /*_*/, -1,       26 /*a*/, 27 /*b*/,
    28 /*c*/, 29 /*d*/, 30 /*e*/, 31 /*f*/,  32 /*g*/, 33 /*h*/, 34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/, 38 /*m*/,
    39 /*n*/, 40 /*o*/, 41 /*p*/, 42 /*q*/,  43 /*r*/, 44 /*s*/, 45 /*t*/, 46 /*u*/, 47 /*v*/, 48 /*w*/, 49 /*x*/,
    50 /*y*/, 51 /*z*/, -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1,       -1,        -1,       -1,       -1,       -1,       -1,       -1,       -1,
    -1,       -1,       -1
};

[[nodiscard]] std::uint32_t Base64Lookup(const char c) {
    // Sign-extend return value so high bit will be set on any unexpected char.
    return static_cast<std::uint32_t>(kBase64UniversalTable[static_cast<uint8_t>(c)]);
}

template <typename Base64Encoder>
std::string Base64Encode(std::string_view data, Pad pad) {
    std::string response;
    try {
        Base64Encoder encoder(new CryptoPP::StringSink(response));
        const CryptoPP::AlgorithmParameters params = CryptoPP::MakeParameters(CryptoPP::Name::Pad(), Pad::kWith == pad)(
            CryptoPP::Name::InsertLineBreaks(),
            false
        );
        encoder.IsolatedInitialize(params);
        encoder.PutMessageEnd(reinterpret_cast<const byte*>(data.data()), data.size());
    } catch (const CryptoPP::Exception& exc) {
        throw crypto::CryptoException(exc.what());
    }
    return response;
}

template <typename Base64Decoder>
std::string Base64Decode(std::string_view data) {
    std::string response;
    try {
        Base64Decoder decoder(new CryptoPP::StringSink(response));
        decoder.PutMessageEnd(reinterpret_cast<const byte*>(data.data()), data.size());
    } catch (const CryptoPP::Exception& exc) {
        throw crypto::CryptoException(exc.what());
    }
    return response;
}

// Unused bits of the last character are not checked: crypto++ ignores them, and so do the SIMD decoders
template <char Char62, char Char63>
bool IsCanonical(std::string_view data) noexcept {
    if (data.size() % 4 == 1) {
        return false;
    }
    const auto chars = data.find_last_not_of('=') + 1;  // 0 if the data is empty or consists of '='
    const auto padding = (data.size() - chars) + (4 - data.size() % 4) % 4;
    if (padding > 2) {
        return false;
    }
    for (const char c : data.substr(0, chars)) {
        const bool is_alphanumeric = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (!is_alphanumeric && c != Char62 && c != Char63) {
            return false;
        }
    }
    return true;
}

// Decodes the alphabet with optional '=' padding at the end, returns std::nullopt for any other input
std::optional<std::string> DecodeCanonical(std::string_view data, impl::Alphabet alphabet) {
#ifndef USERVER_NO_SIMD_BASE64
    return impl::SimdBase64Decode(data, alphabet);
#else
    if (alphabet == impl::Alphabet::kStandard) {
        if (impl::IsCanonicalBase64(data)) {
            return impl::CryptoPPBase64Decode(data);
        }
        return std::nullopt;
    }
#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
    if (impl::IsCanonicalBase64Url(data)) {
        return impl::CryptoPPBase64UrlDecode(data);
    }
#endif
    return std::nullopt;
#endif
}

// As in Go encoding/base64, line breaks are skipped anywhere in the input: Base64 is often wrapped into lines
std::optional<std::string> Decode(std::string_view data, impl::Alphabet alphabet) {
    if (auto result = DecodeCanonical(data, alphabet)) {
        return result;
    }
    if (data.find_first_of("\r\n") == std::string_view::npos) {
        return std::nullopt;
    }
    std::string without_line_breaks;
    without_line_breaks.reserve(data.size());
    for (const char c : data) {
        if (c != '\r' && c != '\n') {
            without_line_breaks.push_back(c);
        }
    }
    return DecodeCanonical(without_line_breaks, alphabet);
}

}  // namespace

namespace impl {

std::string CryptoPPBase64Encode(std::string_view data, Pad pad) {
    return Base64Encode<CryptoPP::Base64Encoder>(data, pad);
}

std::string CryptoPPBase64Decode(std::string_view data) { return Base64Decode<CryptoPP::Base64Decoder>(data); }

bool IsCanonicalBase64(std::string_view data) noexcept { return IsCanonical<'+', '/'>(data); }

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
std::string CryptoPPBase64UrlEncode(std::string_view data, Pad pad) {
    return Base64Encode<CryptoPP::Base64URLEncoder>(data, pad);
}

std::string CryptoPPBase64UrlDecode(std::string_view data) { return Base64Decode<CryptoPP::Base64URLDecoder>(data); }

bool IsCanonicalBase64Url(std::string_view data) noexcept { return IsCanonical<'-', '_'>(data); }
#endif

}  // namespace impl

std::string Base64Encode(std::string_view data, Pad pad) {
#ifndef USERVER_NO_SIMD_BASE64
    return impl::SimdBase64Encode(data, pad, impl::Alphabet::kStandard);
#else
    return impl::CryptoPPBase64Encode(data, pad);
#endif
}

std::string Base64Decode(std::string_view data) {
    if (auto result = Decode(data, impl::Alphabet::kStandard)) {
        return *std::move(result);
    }
    throw CryptoException("Invalid Base64 input");
}

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
std::string Base64UrlEncode(std::string_view data, Pad pad) {
#ifndef USERVER_NO_SIMD_BASE64
    return impl::SimdBase64Encode(data, pad, impl::Alphabet::kUrl);
#else
    return impl::CryptoPPBase64UrlEncode(data, pad);
#endif
}

std::string Base64UrlDecode(std::string_view data) {
    if (auto result = Decode(data, impl::Alphabet::kUrl)) {
        return *std::move(result);
    }
    throw CryptoException("Invalid Base64 URL input");
}
#endif

bool Base64UniversalDecodeInPlace(std::string& data) noexcept {
    // We decode in place. This is safe because this is a new buffer (not
    // aliasing the input) and because base64 decoding shrinks 4 bytes into 3.
    char* out = data.data();
    const char* ptr = data.data();
    const char* end = ptr + data.size();
    const char* end4 = ptr + (data.size() & ~3u);

    for (; ptr < end4; ptr += 4, out += 3) {
        const auto val =
            Base64Lookup(ptr[0]) << 18 | Base64Lookup(ptr[1]) << 12 | Base64Lookup(ptr[2]) << 6 |
            Base64Lookup(ptr[3]) << 0;

        if (static_cast<std::int32_t>(val) < 0) {
            // Junk chars or padding. Remove trailing padding, if any.
            if (end - ptr == 4 && ptr[3] == '=') {
                if (ptr[2] == '=') {
                    end -= 2;
                } else {
                    end -= 1;
                }
            }
            break;
        }

        out[0] = val >> 16;
        out[1] = (val >> 8) & 0xff;
        out[2] = val & 0xff;
    }

    if (ptr < end) {
        std::uint32_t val = ~0u;
        switch (end - ptr) {
            case 2:
                val = Base64Lookup(ptr[0]) << 18 | Base64Lookup(ptr[1]) << 12;
                out[0] = val >> 16;
                out += 1;
                break;
            case 3:
                val = Base64Lookup(ptr[0]) << 18 | Base64Lookup(ptr[1]) << 12 | Base64Lookup(ptr[2]) << 6;
                out[0] = val >> 16;
                out[1] = (val >> 8) & 0xff;
                out += 2;
                break;
        }

        if (static_cast<std::int32_t>(val) < 0) {
            return false;
        }
    }

    data.resize(static_cast<std::size_t>(out - data.data()));
    return true;
}

}  // namespace crypto::base64

USERVER_NAMESPACE_END
