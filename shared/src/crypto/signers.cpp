#include <userver/crypto/signers.hpp>

#include <climits>

#include <cryptopp/dsa.h>
#include <openssl/cms.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <crypto/helpers.hpp>
#include <crypto/openssl.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {
namespace {

// OpenSSL generates ECDSA signatures in ASN.1/DER format, however RFC7518
// specifies signature as a concatenation of zero-padded big-endian `(R, S)`
// values.
std::string ConvertEcSignature(const std::string& der_signature,
                               DigestSize digest_size) {
  size_t siglen = 0;
  switch (digest_size) {
    case DigestSize::k256:
      siglen = 256;
      break;
    case DigestSize::k384:
      siglen = 384;
      break;
    case DigestSize::k512:
      siglen = 521;  // not a typo
      break;
    case DigestSize::k160:
      UINVARIANT(false, "Unexpected DigestSize");
  }
  siglen = ((siglen + CHAR_BIT - 1) / CHAR_BIT) * 2;

  std::string converted_signature(siglen, '\0');
  if (siglen !=
      CryptoPP::DSAConvertSignatureFormat(
          reinterpret_cast<unsigned char*>(converted_signature.data()),
          converted_signature.size(), CryptoPP::DSASignatureFormat::DSA_P1363,
          reinterpret_cast<const unsigned char*>(der_signature.data()),
          der_signature.size(), CryptoPP::DSASignatureFormat::DSA_DER)) {
    throw SignError("Failed to sign: signature format conversion failed");
  }
  return converted_signature;
}

int ToNativeCmsFlags(utils::Flags<CmsSigner::Flags> flags) {
  int native = 0;

  using SignFlags = CmsSigner::Flags;
  if (flags & SignFlags::kText) {
    native |= CMS_TEXT;
  }
  if (flags & SignFlags::kNoCerts) {
    native |= CMS_NOCERTS;
  }
  if (flags & SignFlags::kDetached) {
    native |= CMS_DETACHED;
  }
  if (flags & SignFlags::kBinary) {
    native |= CMS_BINARY;
  }

  return native;
}

void OutputCmsContent(BIO& to, CMS_ContentInfo& cms,
                      CmsSigner::OutForm out_form) {
  using OutForm = CmsSigner::OutForm;
  switch (out_form) {
    case OutForm::kDer: {
      if (!i2d_CMS_bio(&to, &cms)) {
        throw SignError{FormatSslError("Failed to sign: i2d_CMS_bio")};
      }
      break;
    }
    case OutForm::kPem: {
      if (!PEM_write_bio_CMS_stream(&to, &cms, nullptr, 0)) {
        throw SignError{
            FormatSslError("Failed to sign: PEM_write_bio_CMS_stream")};
      }
      break;
    }
    case OutForm::kSMime: {
      if (!SMIME_write_CMS(&to, &cms, nullptr, 0)) {
        throw SignError{FormatSslError("Failed to sign: SMIME_write_CMS")};
      }
      break;
    }
  }
}

}  // namespace

Signer::Signer(const std::string& name) : NamedAlgo(name) {}
Signer::~Signer() = default;

///
/// None
///

SignerNone::SignerNone() : Signer("none") {}
std::string SignerNone::Sign(
    std::initializer_list<std::string_view> /*data*/) const {
  return {};
}

///
/// HMAC-SHA
///

template <DigestSize bits>
HmacShaSigner<bits>::HmacShaSigner(std::string secret)
    : Signer("HS" + EnumValueToString(bits)), secret_(std::move(secret)) {}

template <DigestSize bits>
HmacShaSigner<bits>::~HmacShaSigner() {
  OPENSSL_cleanse(secret_.data(), secret_.size());
}

template <DigestSize bits>
std::string HmacShaSigner<bits>::Sign(
    std::initializer_list<std::string_view> data) const {
  const auto hmac = GetHmacFuncByEnum(bits);

  if (data.size() <= 1) {
    std::string_view single_value{};
    if (data.size()) {
      single_value = *data.begin();
    }

    return hmac(secret_, single_value, crypto::hash::OutputEncoding::kBinary);
  }

  return hmac(secret_, InitListToString(data),
              crypto::hash::OutputEncoding::kBinary);
}

template class HmacShaSigner<DigestSize::k160>;
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
      pkey_(PrivateKey::LoadFromString(key, password)) {
  impl::Openssl::Init();

  if constexpr (type == DsaType::kEc) {
    if (EVP_PKEY_base_id(pkey_.GetNative()) != EVP_PKEY_EC) {
      throw SignError("Non-EC key supplied for " + Name() + " signer");
    }
    if (!IsMatchingKeyCurve(pkey_.GetNative(), bits)) {
      throw SignError("Key curve mismatch for " + Name() + " signer");
    }
  } else {
    if (EVP_PKEY_base_id(pkey_.GetNative()) != EVP_PKEY_RSA) {
      throw SignError("Non-RSA key supplied for " + Name() + " signer");
    }
  }
}

template <DsaType type, DigestSize bits>
std::string DsaSigner<type, bits>::Sign(
    std::initializer_list<std::string_view> data) const {
  EvpMdCtx ctx;
  EVP_PKEY_CTX* pkey_ctx = nullptr;  // non-owning
  if (1 != EVP_DigestSignInit(ctx.Get(), &pkey_ctx, GetShaMdByEnum(bits),
                              nullptr, pkey_.GetNative())) {
    throw SignError(FormatSslError("Failed to sign: EVP_DigestSignInit"));
  }

  if constexpr (type == DsaType::kRsaPss) {
    SetupJwaRsaPssPadding(pkey_ctx, bits);
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
  if (1 != EVP_DigestSignFinal(
               ctx.Get(), reinterpret_cast<unsigned char*>(signature.data()),
               &siglen)) {
    throw SignError(FormatSslError("Failed to sign: EVP_DigestSignFinal"));
  }
  signature.resize(siglen);

  if constexpr (type == DsaType::kEc) {
    return ConvertEcSignature(signature, bits);
  }
  return signature;
}

template <DsaType type, DigestSize bits>
std::string DsaSigner<type, bits>::SignDigest(std::string_view digest) const {
  if constexpr (type == DsaType::kRsaPss) {
    UASSERT_MSG(false, "SignDigest is not available with PSS padding");
    throw CryptoException("SignDigest is not available with PSS padding");
  }

  if (digest.size() != GetDigestLength(bits)) {
    throw SignError("Invalid digest size for " + Name() + " signer");
  }

  const std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> pkey_ctx(
      EVP_PKEY_CTX_new(pkey_.GetNative(), nullptr), EVP_PKEY_CTX_free);
  if (!pkey_ctx) {
    throw SignError(FormatSslError("Failed to sign digest: EVP_PKEY_CTX_new"));
  }
  if (1 != EVP_PKEY_sign_init(pkey_ctx.get())) {
    throw SignError(
        FormatSslError("Failed to sign digest: EVP_PKEY_sign_init"));
  }
  if (EVP_PKEY_CTX_set_signature_md(pkey_ctx.get(), GetShaMdByEnum(bits)) <=
      0) {
    throw SignError(
        FormatSslError("Failed to sign digest: EVP_PKEY_CTX_set_signature_md"));
  }

  size_t siglen = 0;
  if (1 != EVP_PKEY_sign(pkey_ctx.get(), nullptr, &siglen,
                         reinterpret_cast<const unsigned char*>(digest.data()),
                         digest.size())) {
    throw SignError(
        FormatSslError("Failed to sign digest: EVP_PKEY_sign (size check)"));
  }

  std::string signature(siglen, '\0');
  if (1 != EVP_PKEY_sign(pkey_ctx.get(),
                         reinterpret_cast<unsigned char*>(signature.data()),
                         &siglen,
                         reinterpret_cast<const unsigned char*>(digest.data()),
                         digest.size())) {
    throw SignError(FormatSslError("Failed to sign digest: EVP_PKEY_sign"));
  }
  signature.resize(siglen);

  if constexpr (type == DsaType::kEc) {
    return ConvertEcSignature(signature, bits);
  }
  return signature;
}

template class DsaSigner<DsaType::kRsa, DigestSize::k160>;
template class DsaSigner<DsaType::kRsa, DigestSize::k256>;
template class DsaSigner<DsaType::kRsa, DigestSize::k384>;
template class DsaSigner<DsaType::kRsa, DigestSize::k512>;

template class DsaSigner<DsaType::kEc, DigestSize::k256>;
template class DsaSigner<DsaType::kEc, DigestSize::k384>;
template class DsaSigner<DsaType::kEc, DigestSize::k512>;

template class DsaSigner<DsaType::kRsaPss, DigestSize::k160>;
template class DsaSigner<DsaType::kRsaPss, DigestSize::k256>;
template class DsaSigner<DsaType::kRsaPss, DigestSize::k384>;
template class DsaSigner<DsaType::kRsaPss, DigestSize::k512>;

CmsSigner::CmsSigner(Certificate certificate, PrivateKey pkey)
    : NamedAlgo{"CMS"}, cert_{std::move(certificate)}, pkey_{std::move(pkey)} {}

CmsSigner::~CmsSigner() = default;

std::string CmsSigner::Sign(std::initializer_list<std::string_view> data,
                            utils::Flags<Flags> flags, OutForm out_form) const {
  const auto native_flags = ToNativeCmsFlags(flags);

  const auto data_string = InitListToString(data);
  const auto bio_data = MakeBioString(data_string);
  if (!bio_data) {
    throw SignError{FormatSslError("Failed to sign: MakeBioString")};
  }

  const std::unique_ptr<CMS_ContentInfo, decltype(&CMS_ContentInfo_free)>
      cms_content{CMS_sign(cert_.GetNative(), pkey_.GetNative(), nullptr,
                           bio_data.get(), native_flags),
                  CMS_ContentInfo_free};
  if (!cms_content) {
    throw SignError{FormatSslError("Failed to sign: CMS_sign")};
  }

  const std::unique_ptr<BIO, decltype(&BIO_free_all)> bio_output{
      BIO_new(BIO_s_mem()), BIO_free_all};
  if (!bio_output) {
    throw SignError{FormatSslError("Failed to sign: BIO_new(BIO_s_mem())")};
  }

  OutputCmsContent(*bio_output, *cms_content, out_form);

  char* output_data = nullptr;
  // Some openssl macro with c-style (as expected) casts
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  const auto output_len = BIO_get_mem_data(bio_output.get(), &output_data);
  if (!output_data || output_len < 0) {
    throw SignError{FormatSslError("Failed to sign: BIO_get_mem_data")};
  }

  return std::string(output_data, output_len);
}

}  // namespace crypto

USERVER_NAMESPACE_END
