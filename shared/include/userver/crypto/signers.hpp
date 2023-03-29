#pragma once

/// @file userver/crypto/signers.hpp
/// @brief Digital signature generators

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>

#include <userver/utils/flags.hpp>

#include <userver/crypto/basic_types.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/private_key.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

/// Base signer class
class Signer : public NamedAlgo {
 public:
  explicit Signer(const std::string& name);
  ~Signer() override;

  /// Signs a raw message, returning the signature
  virtual std::string Sign(
      std::initializer_list<std::string_view> data) const = 0;
};

/// "none" algorithm signer
class SignerNone final : public Signer {
 public:
  SignerNone();

  /// Signs a raw message, returning the signature
  std::string Sign(std::initializer_list<std::string_view> data) const override;
};

/// HMAC-SHA signer
template <DigestSize bits>
class HmacShaSigner final : public Signer {
 public:
  /// Constructor from a shared secret
  explicit HmacShaSigner(std::string secret);
  ~HmacShaSigner() override;

  /// Signs a raw message, returning the signature
  std::string Sign(std::initializer_list<std::string_view> data) const override;

 private:
  std::string secret_;
};

/// @name Outputs HMAC SHA MAC.
/// @{
using SignerHs1 = HmacShaSigner<DigestSize::k160>;
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

  /// Signs a raw message, returning the signature
  std::string Sign(std::initializer_list<std::string_view> data) const override;

  /// Signs a message digest, returning the signature.
  ///
  /// Not available for RSASSA-PSS.
  std::string SignDigest(std::string_view digest) const;

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

/// @name Outputs RSASSA signature using SHA-2 and PSS padding as per RFC7518.
///
/// JWA specifications require using MGF1 function with the same hash function
/// as for the digest and salt length to be the same size as the hash output.
/// @{
using SignerPs256 = DsaSigner<DsaType::kRsaPss, DigestSize::k256>;
using SignerPs384 = DsaSigner<DsaType::kRsaPss, DigestSize::k384>;
using SignerPs512 = DsaSigner<DsaType::kRsaPss, DigestSize::k512>;
/// @}

/// @name CMS signer per RFC 5652
class CmsSigner final : public NamedAlgo {
 public:
  /// Signer flags
  enum class Flags {
    kNone = 0x0,
    /// If set MIME headers for type text/plain are prepended to the data.
    kText = 0x1,
    /// If set the signer's certificate will not be included in the
    /// resulting CMS structure. This can reduce the size of
    /// the signature if the signers certificate can be obtained by other means:
    /// for example a previously signed message.
    kNoCerts = 0x2,
    /// If set data being signed is omitted from the resulting CMS structure.
    kDetached = 0x40,
    /// Normally the supplied content is translated into MIME canonical format
    /// (as required by the S/MIME specifications).
    /// If set no translation occurs.
    kBinary = 0x80
  };

  /// Output encoding
  enum class OutForm { kDer, kPem, kSMime };

  /// Construct from certificate and private key
  CmsSigner(Certificate certificate, PrivateKey pkey);

  ~CmsSigner() override;

  /// Signs a raw message, returning as specified by flags
  std::string Sign(std::initializer_list<std::string_view> data,
                   utils::Flags<Flags> flags, OutForm out_form) const;

 private:
  Certificate cert_;
  PrivateKey pkey_;
};

namespace weak {

/// Outputs RSASSA signature using SHA-1 and PKCS1 padding.
using SignerRs1 = DsaSigner<DsaType::kRsa, DigestSize::k160>;

/// Outputs RSASSA signature using SHA-2 and PSS padding.
///
/// JWA specifications require using MGF1 function with the same hash function
/// as for the digest and salt length to be the same size as the hash output.
using SignerPs1 = DsaSigner<DsaType::kRsaPss, DigestSize::k160>;

}  // namespace weak
}  // namespace crypto

USERVER_NAMESPACE_END
