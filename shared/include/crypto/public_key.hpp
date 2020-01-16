#pragma once

/// @file crypto/public_key.hpp
/// @brief @copybrief crypto::PublicKey

#include <memory>

#include <crypto/basic_types.hpp>
#include <utils/string_view.hpp>

namespace crypto {

class Certificate;

/// Loaded into memory public key
class PublicKey {
 public:
  using NativeType = EVP_PKEY;

  PublicKey() = default;

  NativeType* GetNative() const noexcept { return pkey_.get(); }
  explicit operator bool() const noexcept { return !!pkey_; }

  /// Accepts a string that contains a certificate or public key, checks that
  /// it's correct, loads it into OpenSSL structures and returns as a
  /// PublicKey variable.
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PublicKey LoadFromString(utils::string_view key);

  /// Extracts PublicKey from certificate.
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PublicKey LoadFromCertificate(const Certificate& cert);

 private:
  explicit PublicKey(std::shared_ptr<NativeType> pkey)
      : pkey_(std::move(pkey)) {}

  std::shared_ptr<NativeType> pkey_;
};

}  // namespace crypto
