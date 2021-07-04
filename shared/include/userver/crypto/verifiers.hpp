#pragma once

/// @file userver/crypto/verifiers.hpp
/// @brief Digital signature verifiers

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>

#include <userver/crypto/basic_types.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/public_key.hpp>

namespace crypto {

/// Base verifier class
class Verifier : public NamedAlgo {
 public:
  explicit Verifier(const std::string& name);
  ~Verifier() override;

  /// Verifies a signature against the message
  virtual void Verify(std::initializer_list<std::string_view> data,
                      std::string_view raw_signature) const = 0;
};

/// "none" algorithm verifier
class VerifierNone final : public Verifier {
 public:
  VerifierNone();

  /// Verifies a signature against the message
  void Verify(std::initializer_list<std::string_view> data,
              std::string_view raw_signature) const override;
};

/// HMAC-SHA verifier
template <DigestSize bits>
class HmacShaVerifier final : public Verifier {
 public:
  /// Constructor from a shared secret
  explicit HmacShaVerifier(std::string secret);
  ~HmacShaVerifier() override;

  /// Verifies a signature against the message
  void Verify(std::initializer_list<std::string_view> data,
              std::string_view raw_signature) const override;

 private:
  std::string secret_;
};

/// @name Verifies HMAC SHA-2 MAC.
/// @{
using VerifierHs256 = HmacShaVerifier<DigestSize::k256>;
using VerifierHs384 = HmacShaVerifier<DigestSize::k384>;
using VerifierHs512 = HmacShaVerifier<DigestSize::k512>;
/// @}

/// Generic verifier for asymmetric cryptography
template <DsaType type, DigestSize bits>
class DsaVerifier final : public Verifier {
 public:
  /// Constructor from a PEM-encoded public key or a X509 certificate
  explicit DsaVerifier(const std::string& pubkey);

  /// Verifies a signature against the message
  void Verify(std::initializer_list<std::string_view> data,
              std::string_view raw_signature) const override;

  /// Verifies a signature against the message digest.
  ///
  /// Not available for RSASSA-PSS.
  /// @warning Do not use this function when the raw message is available!
  void VerifyDigest(std::string_view digest,
                    std::string_view raw_signature) const;

 private:
  PublicKey pkey_;
};

/// @name Verifies RSASSA signature using SHA-2 and PKCS1 padding.
/// @{
using VerifierRs256 = DsaVerifier<DsaType::kRsa, DigestSize::k256>;
using VerifierRs384 = DsaVerifier<DsaType::kRsa, DigestSize::k384>;
using VerifierRs512 = DsaVerifier<DsaType::kRsa, DigestSize::k512>;
/// @}

/// @name Verifies ECDSA as per RFC7518.
///
/// OpenSSL generates ECDSA signatures in ASN.1/DER format, RFC7518 specifies
/// signature as a concatenation of zero-padded big-endian `(R, S)` values.
/// @{
using VerifierEs256 = DsaVerifier<DsaType::kEc, DigestSize::k256>;
using VerifierEs384 = DsaVerifier<DsaType::kEc, DigestSize::k384>;
using VerifierEs512 = DsaVerifier<DsaType::kEc, DigestSize::k512>;
/// @}

/// @name Verifies RSASSA signature using SHA-2 and PSS padding as per RFC7518.
///
/// JWA specifications require using MGF1 function with the same hash function
/// as for the digest and salt length to be the same size as the hash output.
/// @{
using VerifierPs256 = DsaVerifier<DsaType::kRsaPss, DigestSize::k256>;
using VerifierPs384 = DsaVerifier<DsaType::kRsaPss, DigestSize::k384>;
using VerifierPs512 = DsaVerifier<DsaType::kRsaPss, DigestSize::k512>;
/// @}

}  // namespace crypto
