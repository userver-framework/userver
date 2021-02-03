#include <crypto/verifiers.hpp>

#include <cryptopp/dsa.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <crypto/hash.hpp>
#include <crypto/helpers.hpp>
#include <crypto/openssl.hpp>
#include <utils/assert.hpp>

namespace crypto {
namespace {

// OpenSSL expects ECDSA signatures in ASN.1/DER format, however RFC7518
// specifies signature as a concatenation of zero-padded big-endian `(R, S)`
// values.
std::vector<unsigned char> ConvertEcSignature(std::string_view raw_signature) {
  // Must be strictly larger than max signature size in ASN.1/DER format
  constexpr size_t kDerEcdsaSignatureBufferSize = 256;

  std::vector<unsigned char> der_signature(kDerEcdsaSignatureBufferSize, '\0');
  size_t siglen = CryptoPP::DSAConvertSignatureFormat(
      der_signature.data(), der_signature.size(),
      CryptoPP::DSASignatureFormat::DSA_DER,
      reinterpret_cast<const unsigned char*>(raw_signature.data()),
      raw_signature.size(), CryptoPP::DSASignatureFormat::DSA_P1363);
  // 6 is the minimum ASN.1 overhead for a sequence of two integers.
  // Leading zeroes are omitted from the result.
  if (siglen < 6 || siglen >= der_signature.size()) {
    throw VerificationError(
        "Failed to verify digest: signature format conversion failed");
  }
  der_signature.resize(siglen);
  return der_signature;
}

}  // namespace

Verifier::Verifier(const std::string& name) : NamedAlgo(name) {}
Verifier::~Verifier() = default;

///
/// None
///

VerifierNone::VerifierNone() : Verifier("none") {}
void VerifierNone::Verify(std::initializer_list<std::string_view> /*data*/,
                          std::string_view raw_signature) const {
  if (!raw_signature.empty()) throw VerificationError("Signature is not empty");
}

///
/// HMAC-SHA
///

template <DigestSize bits>
HmacShaVerifier<bits>::HmacShaVerifier(std::string secret)
    : Verifier("HS" + EnumValueToString(bits)), secret_(std::move(secret)){};

template <DigestSize bits>
HmacShaVerifier<bits>::~HmacShaVerifier() {
  OPENSSL_cleanse(secret_.data(), secret_.size());
}

template <DigestSize bits>
void HmacShaVerifier<bits>::Verify(std::initializer_list<std::string_view> data,
                                   std::string_view raw_signature) const {
  const auto hmac = GetHmacFuncByEnum(bits);
  std::string signature;
  if (data.size() <= 1) {
    std::string_view single_value{};
    if (data.size() == 1) {
      single_value = *data.begin();
    }

    signature =
        hmac(secret_, single_value, crypto::hash::OutputEncoding::kBinary);
  } else {
    signature = hmac(secret_, InitListToString(data),
                     crypto::hash::OutputEncoding::kBinary);
  }

  if (raw_signature != signature) {
    throw VerificationError("Invalid signature");
  }
}

template class HmacShaVerifier<DigestSize::k256>;
template class HmacShaVerifier<DigestSize::k384>;
template class HmacShaVerifier<DigestSize::k512>;

///
/// *SA
///

template <DsaType type, DigestSize bits>
DsaVerifier<type, bits>::DsaVerifier(const std::string& key)
    : Verifier(EnumValueToString(type) + EnumValueToString(bits)),
      pkey_(PublicKey::LoadFromString(key)) {
  impl::Openssl::Init();

  if constexpr (type == DsaType::kEc) {
    if (EVP_PKEY_base_id(pkey_.GetNative()) != EVP_PKEY_EC) {
      throw VerificationError("Non-EC key supplied for " + Name() +
                              " verifier");
    }
    if (!IsMatchingKeyCurve(pkey_.GetNative(), bits)) {
      throw VerificationError("Key curve mismatch for " + Name() + " verifier");
    }
  } else {
    if (EVP_PKEY_base_id(pkey_.GetNative()) != EVP_PKEY_RSA) {
      throw VerificationError("Non-RSA key supplied for " + Name() +
                              " verifier");
    }
  }
}

template <DsaType type, DigestSize bits>
void DsaVerifier<type, bits>::Verify(
    std::initializer_list<std::string_view> data,
    std::string_view raw_signature) const {
  EvpMdCtx ctx;
  EVP_PKEY_CTX* pkey_ctx = nullptr;
  if (1 != EVP_DigestVerifyInit(ctx.Get(), &pkey_ctx, GetShaMdByEnum(bits),
                                nullptr, pkey_.GetNative())) {
    throw VerificationError(
        FormatSslError("Failed to verify: EVP_DigestVerifyInit"));
  }

  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (type == DsaType::kRsaPss) {
    SetupJwaRsaPssPadding(pkey_ctx, bits);
  }

  for (const auto& part : data) {
    if (1 != EVP_DigestVerifyUpdate(ctx.Get(), part.data(), part.size())) {
      throw VerificationError(
          FormatSslError("Failed to verify: EVP_DigestVerifyUpdate"));
    }
  }

  int verification_result = -1;
  if constexpr (type == DsaType::kEc) {
    auto der_signature = ConvertEcSignature(raw_signature);
    verification_result = EVP_DigestVerifyFinal(ctx.Get(), der_signature.data(),
                                                der_signature.size());
  } else {
    verification_result = EVP_DigestVerifyFinal(
        ctx.Get(), reinterpret_cast<const unsigned char*>(raw_signature.data()),
        raw_signature.size());
  }

  if (1 != verification_result) {
    throw VerificationError(
        FormatSslError("Failed to verify: EVP_DigestVerifyFinal"));
  }
}

template <DsaType type, DigestSize bits>
void DsaVerifier<type, bits>::VerifyDigest(
    std::string_view digest, std::string_view raw_signature) const {
  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (type == DsaType::kRsaPss) {
    UASSERT_MSG(false, "VerifyDigest is not available with PSS padding");
    throw CryptoException("VerifyDigest is not available with PSS padding");
  }

  if (digest.size() != GetDigestLength(bits)) {
    throw VerificationError("Invalid digest size for " + Name() + " verifier");
  }

  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> pkey_ctx(
      EVP_PKEY_CTX_new(pkey_.GetNative(), nullptr), EVP_PKEY_CTX_free);
  if (!pkey_ctx) {
    throw VerificationError(
        FormatSslError("Failed to verify digest: EVP_PKEY_CTX_new"));
  }
  if (1 != EVP_PKEY_verify_init(pkey_ctx.get())) {
    throw VerificationError(
        FormatSslError("Failed to verify digest: EVP_PKEY_verify_init"));
  }
  if (EVP_PKEY_CTX_set_signature_md(pkey_ctx.get(), GetShaMdByEnum(bits)) <=
      0) {
    throw VerificationError(
        FormatSslError("Failed to sign digest: EVP_PKEY_CTX_set_signature_md"));
  }

  int verification_result = -1;
  if constexpr (type == DsaType::kEc) {
    auto der_signature = ConvertEcSignature(raw_signature);
    verification_result = EVP_PKEY_verify(
        pkey_ctx.get(), der_signature.data(), der_signature.size(),
        reinterpret_cast<const unsigned char*>(digest.data()), digest.size());
  } else {
    verification_result = EVP_PKEY_verify(
        pkey_ctx.get(),
        reinterpret_cast<const unsigned char*>(raw_signature.data()),
        raw_signature.size(),
        reinterpret_cast<const unsigned char*>(digest.data()), digest.size());
  }

  if (1 != verification_result) {
    throw VerificationError(
        FormatSslError("Failed to verify digest: EVP_DigestVerifyFinal"));
  }
}

template class DsaVerifier<DsaType::kRsa, DigestSize::k256>;
template class DsaVerifier<DsaType::kRsa, DigestSize::k384>;
template class DsaVerifier<DsaType::kRsa, DigestSize::k512>;

template class DsaVerifier<DsaType::kEc, DigestSize::k256>;
template class DsaVerifier<DsaType::kEc, DigestSize::k384>;
template class DsaVerifier<DsaType::kEc, DigestSize::k512>;

template class DsaVerifier<DsaType::kRsaPss, DigestSize::k256>;
template class DsaVerifier<DsaType::kRsaPss, DigestSize::k384>;
template class DsaVerifier<DsaType::kRsaPss, DigestSize::k512>;

}  // namespace crypto
