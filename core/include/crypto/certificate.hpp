#pragma once

#include <memory>

#include <crypto/basic_types.hpp>
#include <utils/string_view.hpp>

namespace crypto {

/// Loaded into memory X509 certificate
class Certificate {
 public:
  using NativeType = X509;

  Certificate() = default;

  NativeType* GetNative() const noexcept { return cert_.get(); }
  explicit operator bool() const noexcept { return !!cert_; }

  /// Accepts a string that contains a certificate, checks that
  /// it's correct, loads it into OpenSSL structures and returns as a
  /// Certificate variable.
  ///
  /// @throw crypto::KeyParseError if failed to load the certificate.
  static Certificate LoadFromString(utils::string_view certificate);

 private:
  explicit Certificate(std::shared_ptr<NativeType> cert)
      : cert_(std::move(cert)) {}

  std::shared_ptr<NativeType> cert_;
};

}  // namespace crypto
