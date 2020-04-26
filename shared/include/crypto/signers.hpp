#pragma once

/// @file crypto/signers.hpp
/// @brief Digital signature generators

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>

#include <crypto/basic_types.hpp>
#include <crypto/exception.hpp>
#include <crypto/private_key.hpp>

namespace crypto {

/// Base signer class
class Signer : public NamedAlgo {
 public:
  explicit Signer(const std::string& name);
  virtual ~Signer();

  virtual std::string Sign(
      std::initializer_list<std::string_view> data) const = 0;
};

/// "none" algorithm signer
class SignerNone final : public Signer {
 public:
  SignerNone();

  std::string Sign(std::initializer_list<std::string_view> data) const override;
};

/// HMAC-SHA signer
template <DigestSize bits>
class HmacShaSigner final : public Signer {
 public:
  /// Constructor from a shared secret
  explicit HmacShaSigner(std::string secret);
  virtual ~HmacShaSigner();

  std::string Sign(std::initializer_list<std::string_view> data) const override;

 private:
  std::string secret_;
};

/// @name Outputs HMAC SHA-2 MAC.
/// @{
using SignerHs256 = HmacShaSigner<DigestSize::k256>;
using SignerHs384 = HmacShaSigner<DigestSize::k384>;
using SignerHs512 = HmacShaSigner<DigestSize::k512>;
/// @}

/// Generic signer for asymmetric cryptography
template <DsaType type, DigestSize bits>
class DsaSigner final : public Signer {
 public:
  /// Constructor from a PEM-encoded private key and an optional passphrase
  explicit DsaSigner(const std::string& privkey,
                     const std::string& password = {});

  std::string Sign(std::initializer_list<std::string_view> data) const override;

 private:
  PrivateKey pkey_;
};

/// @name Outputs RSASSA signature using SHA-2 and PKCS1 padding.
/// @{
using SignerRs256 = DsaSigner<DsaType::kRsa, DigestSize::k256>;
using SignerRs384 = DsaSigner<DsaType::kRsa, DigestSize::k384>;
using SignerRs512 = DsaSigner<DsaType::kRsa, DigestSize::k512>;
/// @}

/// @name Outputs ECDSA as per RFC7518.
///
/// OpenSSL generates ECDSA signatures in ASN.1/DER format, RFC7518 specifies
/// signature as a concatenation of zero-padded big-endian `(R, S)` values.
/// @{
using SignerEs256 = DsaSigner<DsaType::kEc, DigestSize::k256>;
using SignerEs384 = DsaSigner<DsaType::kEc, DigestSize::k384>;
using SignerEs512 = DsaSigner<DsaType::kEc, DigestSize::k512>;
/// @}

/// @name Outputs RSASSA signature using SHA-2 and PSS padding.
/// @{
using SignerPs256 = DsaSigner<DsaType::kRsaPss, DigestSize::k256>;
using SignerPs384 = DsaSigner<DsaType::kRsaPss, DigestSize::k384>;
using SignerPs512 = DsaSigner<DsaType::kRsaPss, DigestSize::k512>;
/// @}

}  // namespace crypto
