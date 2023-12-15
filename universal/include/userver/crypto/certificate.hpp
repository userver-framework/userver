#pragma once

/// @file userver/crypto/certificate.hpp
/// @brief @copybrief crypto::Certificate

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <userver/crypto/basic_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

/// @ingroup userver_universal userver_containers
///
/// Loaded into memory X509 certificate
class Certificate {
 public:
  using NativeType = X509;

  Certificate() = default;

  NativeType* GetNative() const noexcept { return cert_.get(); }
  explicit operator bool() const noexcept { return !!cert_; }

  /// Returns a PEM-encoded representation of stored certificate.
  ///
  /// @throw crypto::SerializationError if serialization fails.
  std::optional<std::string> GetPemString() const;

  /// Accepts a string that contains a certificate, checks that
  /// it's correct, loads it into OpenSSL structures and returns as a
  /// Certificate variable.
  ///
  /// @throw crypto::KeyParseError if failed to load the certificate.
  static Certificate LoadFromString(std::string_view certificate);

 private:
  explicit Certificate(std::shared_ptr<NativeType> cert)
      : cert_(std::move(cert)) {}

  std::shared_ptr<NativeType> cert_;
};

}  // namespace crypto

USERVER_NAMESPACE_END
