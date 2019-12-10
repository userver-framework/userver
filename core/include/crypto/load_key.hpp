#pragma once

#include <memory>

#include <crypto/basic_types.hpp>
#include <utils/string_view.hpp>

namespace crypto {

/// Accepts a string that contains a private key and a password, checks the key
/// and password, loads it into OpenSSL structures and returns a shared pointer
/// to it.
///
/// @throw crypto::KeyParseError if failed to load the key.
std::shared_ptr<EVP_PKEY> LoadPrivateKeyFromString(utils::string_view key,
                                                   utils::string_view password);

/// Accepts a string that contains a certificate or public key, checks that it's
/// correct, loads it into OpenSSL structures and returns a shared pointer to
/// it.
///
/// @throw crypto::KeyParseError if failed to load the key.
std::shared_ptr<EVP_PKEY> LoadPublicKeyFromString(utils::string_view key);

}  // namespace crypto
