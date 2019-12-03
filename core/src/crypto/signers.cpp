#include <crypto/signers.hpp>

#include <climits>

#include <cryptopp/dsa.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <crypto/hash.hpp>

#include <crypto/helpers.hpp>

namespace crypto {
namespace {

constexpr size_t GetEcdsaSignatureLength(DigestSize digest_size) {
  size_t bits = 0;
  switch (digest_size) {
    case DigestSize::k256:
      bits = 256;
      break;
    case DigestSize::k384:
      bits = 384;
      break;
    case DigestSize::k512:
      bits = 521;  // not a typo
      break;
  }
  return ((bits + CHAR_BIT - 1) / CHAR_BIT) * 2;
}

}  // namespace

Signer::Signer(const std::string& name) : NamedAlgo(name) {}
Signer::~Signer() = default;

///
/// None
///

SignerNone::SignerNone() : Signer("none") {}
std::string SignerNone::Sign(
    std::initializer_list<utils::string_view> /*data*/) const {
  return {};
}

///
/// HMAC-SHA
///

template <DigestSize bits>
HmacShaSigner<bits>::HmacShaSigner(std::string secret)
    : Signer("HS" + EnumValueToString(bits)), secret_(std::move(secret)){};

template <DigestSize bits>
HmacShaSigner<bits>::~HmacShaSigner() {
  OPENSSL_cleanse(&secret_[0], secret_.size());
}

template <DigestSize bits>
std::string HmacShaSigner<bits>::Sign(
    std::initializer_list<utils::string_view> data) const {
  const auto hmac = GetHmacFuncByEnum(bits);

  if (data.size() <= 1) {
    utils::string_view single_value{};
    if (data.size()) {
      single_value = *data.begin();
    }

    return hmac(secret_, single_value, crypto::hash::OutputEncoding::kBinary);
  }

  return hmac(secret_, InitListToString(data),
              crypto::hash::OutputEncoding::kBinary);
}

template class HmacShaSigner<DigestSize::k256>;
template class HmacShaSigner<DigestSize::k384>;
template class HmacShaSigner<DigestSize::k512>;

///
/// *SA
///

template <DsaType type, DigestSize bits>
DsaSigner<type, bits>::DsaSigner(const std::string& key,
                                 const std::string& password)
    : Signer(EnumValueToString(type) + EnumValueToString(bits)),
      pkey_(LoadPrivateKeyFromString(key, password)) {
  if constexpr (type == DsaType::kEc) {
    if (EVP_PKEY_base_id(pkey_.get()) != EVP_PKEY_EC) {
      throw SignError("Non-EC key supplied for " + Name() + " signer");
    }
    if (!IsMatchingKeyCurve(pkey_.get(), bits)) {
      throw SignError("Key curve mismatch for " + Name() + " signer");
    }
  } else {
    if (EVP_PKEY_base_id(pkey_.get()) != EVP_PKEY_RSA) {
      throw SignError("Non-RSA key supplied for " + Name() + " signer");
    }
  }
}

template <DsaType type, DigestSize bits>
std::string DsaSigner<type, bits>::Sign(
    std::initializer_list<utils::string_view> data) const {
  EvpMdCtx ctx;
  EVP_PKEY_CTX* pkey_ctx = nullptr;
  if (1 != EVP_DigestSignInit(ctx.Get(), &pkey_ctx, GetShaMdByEnum(bits),
                              nullptr, pkey_.get())) {
    throw SignError(FormatSslError("Failed to sign: EVP_DigestSignInit"));
  }

  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (type == DsaType::kRsaPss) {
    if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING) <= 0) {
      throw SignError(
          FormatSslError("Failed to sign: EVP_PKEY_CTX_set_rsa_padding"));
    }
  }

  for (const auto& part : data) {
    if (1 != EVP_DigestSignUpdate(ctx.Get(), part.data(), part.size())) {
      throw SignError(FormatSslError("Failed to sign: EVP_DigestSignUpdate"));
    }
  }

  size_t siglen = 0;
  if (1 != EVP_DigestSignFinal(ctx.Get(), nullptr, &siglen)) {
    throw SignError(
        FormatSslError("Failed to sign: EVP_DigestSignFinal (size check)"));
  }

  std::string signature(siglen, '\0');
  if (1 != EVP_DigestSignFinal(ctx.Get(),
                               reinterpret_cast<unsigned char*>(&signature[0]),
                               &siglen)) {
    throw SignError(FormatSslError("Failed to sign: EVP_DigestSignFinal"));
  }
  signature.resize(siglen);

  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (type == DsaType::kEc) {
    // OpenSSL generates ECDSA signatures in ASN.1/DER format, however RFC7518
    // specifies signature as a concatenation of zero-padded big-endian `(R, S)`
    // values.
    std::string converted_signature(GetEcdsaSignatureLength(bits), '\0');
    if (converted_signature.size() !=
        CryptoPP::DSAConvertSignatureFormat(
            reinterpret_cast<unsigned char*>(&converted_signature[0]),
            converted_signature.size(), CryptoPP::DSASignatureFormat::DSA_P1363,
            reinterpret_cast<const unsigned char*>(signature.data()),
            signature.size(), CryptoPP::DSASignatureFormat::DSA_DER)) {
      throw SignError("Failed to sign: signature format conversion failed");
    }
    signature = std::move(converted_signature);
  }

  return signature;
}

template class DsaSigner<DsaType::kRsa, DigestSize::k256>;
template class DsaSigner<DsaType::kRsa, DigestSize::k384>;
template class DsaSigner<DsaType::kRsa, DigestSize::k512>;

template class DsaSigner<DsaType::kEc, DigestSize::k256>;
template class DsaSigner<DsaType::kEc, DigestSize::k384>;
template class DsaSigner<DsaType::kEc, DigestSize::k512>;

template class DsaSigner<DsaType::kRsaPss, DigestSize::k256>;
template class DsaSigner<DsaType::kRsaPss, DigestSize::k384>;
template class DsaSigner<DsaType::kRsaPss, DigestSize::k512>;

}  // namespace crypto
