#pragma once

/// @file userver/crypto/public_key.hpp
/// @brief @copybrief crypto::PublicKey

#include <memory>
#include <string_view>

#include <userver/crypto/basic_types.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

class Certificate;

/// Loaded into memory public key
class PublicKey {
 public:
  using NativeType = EVP_PKEY;

  /// Modulus wrapper
  using ModulusView = utils::StrongTypedef<class ModulusTag, std::string_view>;

  /// Exponent wrapper
  using ExponentView =
      utils::StrongTypedef<class ExponentTag, std::string_view>;

  using CoordinateView =
      utils::StrongTypedef<class CoordinateTag, std::string_view>;

  using CurveTypeView =
      utils::StrongTypedef<class CurveTypeTag, std::string_view>;

  PublicKey() = default;

  NativeType* GetNative() const noexcept { return pkey_.get(); }
  explicit operator bool() const noexcept { return !!pkey_; }

  /// Accepts a string that contains a certificate or public key, checks that
  /// it's correct, loads it into OpenSSL structures and returns as a
  /// PublicKey variable.
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PublicKey LoadFromString(std::string_view key);

  /// Extracts PublicKey from certificate.
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PublicKey LoadFromCertificate(const Certificate& cert);

  /// Creates RSA PublicKey from components
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PublicKey LoadRSAFromComponents(ModulusView modulus,
                                         ExponentView exponent);

  /// Creates EC PublicKey from components
  ///
  /// @throw crypto::KeyParseError if failed to load the key.
  static PublicKey LoadECFromComponents(CurveTypeView curve, CoordinateView x,
                                        CoordinateView y);

 private:
  explicit PublicKey(std::shared_ptr<NativeType> pkey)
      : pkey_(std::move(pkey)) {}

  std::shared_ptr<NativeType> pkey_;
};

}  // namespace crypto

USERVER_NAMESPACE_END
