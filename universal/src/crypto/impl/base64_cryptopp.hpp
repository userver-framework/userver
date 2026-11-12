#pragma once

#include <string>
#include <string_view>

#include <userver/crypto/base64.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto::base64::impl {

std::string CryptoPPBase64Encode(std::string_view data, Pad pad);
/// crypto++ skips characters outside of the alphabet, so the input should be checked with IsCanonicalBase64 first
std::string CryptoPPBase64Decode(std::string_view data);

/// Checks that @a data consists of the standard alphabet with optional '=' padding at the end. Padding may be
/// omitted, but the length without it must not be 1 modulo 4.
bool IsCanonicalBase64(std::string_view data) noexcept;

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
std::string CryptoPPBase64UrlEncode(std::string_view data, Pad pad);
std::string CryptoPPBase64UrlDecode(std::string_view data);

/// Same as IsCanonicalBase64, but for the URL alphabet
bool IsCanonicalBase64Url(std::string_view data) noexcept;
#endif

}  // namespace crypto::base64::impl

USERVER_NAMESPACE_END
