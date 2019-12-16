#pragma once

#include <memory>

#include <crypto/basic_types.hpp>
#include <utils/string_view.hpp>

namespace crypto {

/// Loaded into memory private key
class PrivateKey {
 public:
  using NativeType = EVP_PKEY;

  PrivateKey() = default;

  NativeType* GetNative() const noexcept { return pkey_.get(); }
  explicit operator bool() const noexcept { return !!pkey_; }

  /// Accepts a string that contains a private key and a password, checks the
  /// key and password, loads it into OpenSSL structures and returns as a
  /// PrivateKey variable.
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PrivateKey LoadFromString(utils::string_view key,
                                   utils::string_view password);

  /// Accepts a string that contains a private key (not protected with
  /// password), checks the key and password, loads it into OpenSSL structures
  /// and returns as a PrivateKey variable.
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PrivateKey LoadFromString(utils::string_view key);

 private:
  explicit PrivateKey(std::shared_ptr<NativeType> pkey)
      : pkey_(std::move(pkey)) {}

  std::shared_ptr<NativeType> pkey_{};
};

}  // namespace crypto
