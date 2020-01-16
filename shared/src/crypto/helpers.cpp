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

#include <boost/algorithm/string/predicate.hpp>

#include <crypto/base64.hpp>
#include <crypto/exception.hpp>
#include <crypto/hash.hpp>

namespace crypto {
namespace {

int CurveNidByDigestSize(DigestSize bits) {
  switch (bits) {
    case DigestSize::k256:
      return NID_X9_62_prime256v1;
    case DigestSize::k384:
      return NID_secp384r1;
    case DigestSize::k512:
      return NID_secp521r1;
  }
}

}  // namespace
std::string FormatSslError(std::string message) {
  bool first = true;
  unsigned long ssl_error = 0;

  while ((ssl_error = ERR_get_error()) != 0) {
    const char* reason = ERR_reason_error_string(ssl_error);
    if (!reason) continue;

    if (first) {
      message += ": ";
      first = false;
    } else {
      message += "; ";
    }
    message += reason;
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
    case DigestSize::k256:
      return crypto::hash::HmacSha256;
    case DigestSize::k384:
      return crypto::hash::HmacSha384;
    case DigestSize::k512:
      return crypto::hash::HmacSha512;
  }
}

const EVP_MD* GetShaMdByEnum(DigestSize bits) {
  switch (bits) {
    case DigestSize::k256:
      return EVP_sha256();
    case DigestSize::k384:
      return EVP_sha384();
    case DigestSize::k512:
      return EVP_sha512();
  }
}

// TODO: remove after finishing the https://st.yandex-team.ru/TAXICOMMON-1754
std::string InitListToString(std::initializer_list<utils::string_view> data) {
  std::string combined_value;

  const auto combined_value_size =
      std::accumulate(data.begin(), data.end(), std::size_t{0},
                      [](std::size_t len, utils::string_view value) {
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
}

std::string EnumValueToString(DigestSize bits) {
  switch (bits) {
    case DigestSize::k256:
      return "256";
    case DigestSize::k384:
      return "384";
    case DigestSize::k512:
      return "512";
  }
}

bool IsMatchingKeyCurve(EVP_PKEY* pkey, DigestSize bits) {
  std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)> ec_key(
      EVP_PKEY_get1_EC_KEY(pkey), EC_KEY_free);
  return ec_key && EC_GROUP_get_curve_name(EC_KEY_get0_group(ec_key.get())) ==
                       CurveNidByDigestSize(bits);
}

std::unique_ptr<::BIO, decltype(&::BIO_free_all)> MakeBioString(
    utils::string_view str) {
  return {::BIO_new_mem_buf(str.data(), str.size()), &::BIO_free_all};
}

}  // namespace crypto
