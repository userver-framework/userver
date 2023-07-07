#include <crypto/helpers.hpp>

#include <cstring>
#include <numeric>

#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <boost/algorithm/string/predicate.hpp>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {
namespace {

// undefined in openssl 1.0.x
#ifndef RSA_PSS_SALTLEN_DIGEST
constexpr int RSA_PSS_SALTLEN_DIGEST = -1;
#endif

int CurveNidByDigestSize(DigestSize bits) {
  switch (bits) {
    case DigestSize::k256:
      return NID_X9_62_prime256v1;
    case DigestSize::k384:
      return NID_secp384r1;
    case DigestSize::k512:
      return NID_secp521r1;
    case DigestSize::k160:
      break;  // not supported
  }

  UINVARIANT(false, "Unexpected DigestSize");
}

}  // namespace
std::string FormatSslError(std::string message) {
  bool first = true;
  unsigned long ssl_error = 0;

  while ((ssl_error = ERR_get_error()) != 0) {
    if (first) {
      message += ": ";
      first = false;
    } else {
      message += "; ";
    }

    const char* reason = ERR_reason_error_string(ssl_error);
    if (reason) {
      message += reason;
    } else {
      message += fmt::format(FMT_COMPILE("code {:X}"), ssl_error);
    }
  }
  return message;
}

EvpMdCtx::EvpMdCtx()
    : ctx_(
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
          EVP_MD_CTX_new()
#else
          EVP_MD_CTX_create()
#endif
      ) {
  if (!ctx_) {
    throw CryptoException(FormatSslError("Failed to create EVP_MD_CTX"));
  }
}

EvpMdCtx::~EvpMdCtx() {
  if (ctx_) {
#if OPENSSL_VERSION_NUMBER >= 0x010100000L
    EVP_MD_CTX_free(ctx_);
#else
    EVP_MD_CTX_destroy(ctx_);
#endif
  }
}

EvpMdCtx::EvpMdCtx(EvpMdCtx&& other) noexcept
    : ctx_(std::exchange(other.ctx_, nullptr)) {}

decltype(&crypto::hash::HmacSha256) GetHmacFuncByEnum(DigestSize bits) {
  switch (bits) {
    case DigestSize::k160:
      return crypto::hash::HmacSha1;
    case DigestSize::k256:
      return crypto::hash::HmacSha256;
    case DigestSize::k384:
      return crypto::hash::HmacSha384;
    case DigestSize::k512:
      return crypto::hash::HmacSha512;
  }

  UINVARIANT(false, "Unexpected DigestSize");
}

const EVP_MD* GetShaMdByEnum(DigestSize bits) {
  switch (bits) {
    case DigestSize::k160:
      return EVP_sha1();
    case DigestSize::k256:
      return EVP_sha256();
    case DigestSize::k384:
      return EVP_sha384();
    case DigestSize::k512:
      return EVP_sha512();
  }

  UINVARIANT(false, "Unexpected DigestSize");
}

// TODO: remove after finishing the https://st.yandex-team.ru/TAXICOMMON-1754
std::string InitListToString(std::initializer_list<std::string_view> data) {
  std::string combined_value;

  const auto combined_value_size =
      std::accumulate(data.begin(), data.end(), std::size_t{0},
                      [](std::size_t len, std::string_view value) {
                        return len + value.size();
                      });

  combined_value.reserve(combined_value_size);
  for (const auto& v : data) {
    combined_value.append(v.begin(), v.end());
  }

  return combined_value;
}

std::string EnumValueToString(DsaType type) {
  switch (type) {
    case DsaType::kRsa:
      return "RS";
    case DsaType::kEc:
      return "ES";
    case DsaType::kRsaPss:
      return "PS";
  }

  UINVARIANT(false, "Unexpected DsaType");
}

std::string EnumValueToString(DigestSize bits) {
  switch (bits) {
    case DigestSize::k160:
      return "1";
    case DigestSize::k256:
      return "256";
    case DigestSize::k384:
      return "384";
    case DigestSize::k512:
      return "512";
  }

  UINVARIANT(false, "Unexpected DigestSize");
}

bool IsMatchingKeyCurve(EVP_PKEY* pkey, DigestSize bits) {
  std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)> ec_key(
      EVP_PKEY_get1_EC_KEY(pkey), EC_KEY_free);
  return ec_key && EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key.get())) ==
                       CurveNidByDigestSize(bits);
}

std::unique_ptr<::BIO, decltype(&::BIO_free_all)> MakeBioString(
    std::string_view str) {
  return {::BIO_new_mem_buf(str.data(), str.size()), &::BIO_free_all};
}

void SetupJwaRsaPssPadding(EVP_PKEY_CTX* pkey_ctx, DigestSize bits) {
  if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING) <= 0) {
    throw CryptoException(FormatSslError(
        "Failed to setup PSS padding: EVP_PKEY_CTX_set_rsa_padding"));
  }
  if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, RSA_PSS_SALTLEN_DIGEST) <= 0) {
    throw CryptoException(FormatSslError(
        "Failed to setup PSS padding: EVP_PKEY_CTX_set_rsa_pss_saltlen"));
  }
  if (EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, GetShaMdByEnum(bits)) <= 0) {
    throw CryptoException(FormatSslError(
        "Failed to setup PSS padding: EVP_PKEY_CTX_set_rsa_mgf1_md"));
  }
}

}  // namespace crypto

USERVER_NAMESPACE_END
