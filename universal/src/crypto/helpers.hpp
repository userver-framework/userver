#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <userver/crypto/basic_types.hpp>
#include <userver/crypto/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

std::string FormatSslError(std::string message);

class EvpMdCtx {
 public:
  EvpMdCtx();
  ~EvpMdCtx();

  EvpMdCtx(const EvpMdCtx&) = delete;
  EvpMdCtx(EvpMdCtx&&) noexcept;

  EVP_MD_CTX* Get() { return ctx_; }

 private:
  EVP_MD_CTX* ctx_;
};

constexpr size_t GetDigestLength(DigestSize digest_size) {
  size_t bits = 0;
  switch (digest_size) {
    case DigestSize::k160:
      bits = 160;
      break;
    case DigestSize::k256:
      bits = 256;
      break;
    case DigestSize::k384:
      bits = 384;
      break;
    case DigestSize::k512:
      bits = 512;
      break;
  }
  return (bits + CHAR_BIT - 1) / CHAR_BIT;
}

decltype(&crypto::hash::HmacSha256) GetHmacFuncByEnum(DigestSize bits);
const EVP_MD* GetShaMdByEnum(DigestSize bits);

std::string InitListToString(std::initializer_list<std::string_view> data);

std::string EnumValueToString(DsaType type);
std::string EnumValueToString(DigestSize bits);

bool IsMatchingKeyCurve(EVP_PKEY*, DigestSize bits);

std::unique_ptr<::BIO, decltype(&::BIO_free_all)> MakeBioString(
    std::string_view str);

void SetupJwaRsaPssPadding(EVP_PKEY_CTX*, DigestSize bits);

}  // namespace crypto

USERVER_NAMESPACE_END
