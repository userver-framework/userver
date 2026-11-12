#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <userver/crypto/base64.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto::base64::impl {

enum class Alphabet { kStandard, kUrl };

// Defined only if userver is built with aklomp/base64 (USERVER_NO_SIMD_BASE64 is not defined)

/// Encodes with aklomp/base64.
std::string SimdBase64Encode(std::string_view data, Pad pad, Alphabet alphabet);

/// Decodes canonical input with aklomp/base64: the alphabet with optional '=' padding at the end. Returns
/// std::nullopt for any other input, the same input that IsCanonicalBase64 rejects.
std::optional<std::string> SimdBase64Decode(std::string_view data, Alphabet alphabet);

}  // namespace crypto::base64::impl

USERVER_NAMESPACE_END
