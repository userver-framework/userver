#include <userver/crypto/verifiers.hpp>

#include <cryptopp/dsa.h>

#include <openssl/pem.h>
// keep these two headers in this order
#include <openssl/cms.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#include <crypto/helpers.hpp>
#include <crypto/openssl.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

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

int ToNativeCmsFlags(utils::Flags<CmsVerifier::Flags> flags) {
  int native = 0;

  using VerifyFlags = CmsVerifier::Flags;
  if (flags & VerifyFlags::kNoSignerCertVerify) {
    native |= CMS_NO_SIGNER_CERT_VERIFY;
  }

  return native;
}

std::unique_ptr<CMS_ContentInfo, decltype(&CMS_ContentInfo_free)>
ReadCmsContent(BIO& from, CmsVerifier::InForm in_form) {
  using InForm = CmsVerifier::InForm;

  std::unique_ptr<CMS_ContentInfo, decltype(&CMS_ContentInfo_free)> cms{
      nullptr, CMS_ContentInfo_free};
  switch (in_form) {
    case InForm::kDer: {
      cms.reset(d2i_CMS_bio(&from, nullptr));
      if (!cms) {
        throw VerificationError{
            FormatSslError("Failed to verify: d2i_CMS_bio")};
      }
      break;
    }
    case InForm::kPem: {
      cms.reset(PEM_read_bio_CMS(&from, nullptr, nullptr, nullptr));
      if (!cms) {
        throw VerificationError{
            FormatSslError("Failed to verify: PEM_read_bio_CMS")};
      }
      break;
    }
    case InForm::kSMime: {
      cms.reset(SMIME_read_CMS(&from, nullptr));
      if (!cms) {
        throw VerificationError{
            FormatSslError("Failed to verify: SMIME_read_CMS")};
      }
      break;
    }
  }

  UASSERT(cms);
  return cms;
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
    : Verifier("HS" + EnumValueToString(bits)), secret_(std::move(secret)) {}

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

template class HmacShaVerifier<DigestSize::k160>;
template class HmacShaVerifier<DigestSize::k256>;
template class HmacShaVerifier<DigestSize::k384>;
template class HmacShaVerifier<DigestSize::k512>;

///
/// *SA
///

template <DsaType type, DigestSize bits>
DsaVerifier<type, bits>::DsaVerifier(PublicKey pubkey)
    : Verifier(EnumValueToString(type) + EnumValueToString(bits)),
      pkey_(std::move(pubkey)) {
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
DsaVerifier<type, bits>::DsaVerifier(std::string_view key)
    : DsaVerifier{PublicKey::LoadFromString(key)} {}

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

template class DsaVerifier<DsaType::kRsa, DigestSize::k160>;
template class DsaVerifier<DsaType::kRsa, DigestSize::k256>;
template class DsaVerifier<DsaType::kRsa, DigestSize::k384>;
template class DsaVerifier<DsaType::kRsa, DigestSize::k512>;

template class DsaVerifier<DsaType::kEc, DigestSize::k256>;
template class DsaVerifier<DsaType::kEc, DigestSize::k384>;
template class DsaVerifier<DsaType::kEc, DigestSize::k512>;

template class DsaVerifier<DsaType::kRsaPss, DigestSize::k160>;
template class DsaVerifier<DsaType::kRsaPss, DigestSize::k256>;
template class DsaVerifier<DsaType::kRsaPss, DigestSize::k384>;
template class DsaVerifier<DsaType::kRsaPss, DigestSize::k512>;

CmsVerifier::CmsVerifier(Certificate certificate)
    : NamedAlgo{"CMS"}, cert_{std::move(certificate)} {}

CmsVerifier::~CmsVerifier() = default;

void CmsVerifier::Verify(std::initializer_list<std::string_view> data,
                         utils::Flags<Flags> flags, InForm in_form) const {
  const auto native_flags = ToNativeCmsFlags(flags);

  const auto data_string = InitListToString(data);
  const auto bio_data = MakeBioString(data_string);
  if (!bio_data) {
    throw VerificationError{FormatSslError("Failed to verify: MakeBioString")};
  }

  const auto cms_content = ReadCmsContent(*bio_data, in_form);

  using CertStack = STACK_OF(X509);
  // sk_X509_free is a macros in libssl 3.0+,
  // thus decltype(&sk_X509_free) doesn't work, so this.
  const auto stack_deleter = [](STACK_OF(X509) * sk) { sk_X509_free(sk); };
  const std::unique_ptr<CertStack, decltype(stack_deleter)> certs{
      sk_X509_new_reserve(nullptr, 1), stack_deleter};
  if (!certs) {
    throw VerificationError{
        FormatSslError("Failed to verify: sk_X509_new_reserve")};
  }

  if (sk_X509_push(certs.get(), cert_.GetNative()) != 1) {
    // openssl guarantees that this can't happen if new_reserve succeeds,
    // but we're better off checking anyway.
    throw VerificationError{FormatSslError("Failed to verify: sk_X509_push")};
  }

  if (1 != CMS_verify(cms_content.get(), certs.get(),                 //
                      nullptr /* store */,                            //
                      nullptr /* dcont, detached content that is */,  //
                      nullptr /* out */,                              //
                      native_flags)) {
    throw VerificationError{FormatSslError("Failed to verify: CMS_verify")};
  }
}

}  // namespace crypto

USERVER_NAMESPACE_END
