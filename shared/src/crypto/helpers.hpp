#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <crypto/basic_types.hpp>
#include <crypto/hash.hpp>

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

decltype(&crypto::hash::HmacSha256) GetHmacFuncByEnum(DigestSize bits);
const EVP_MD* GetShaMdByEnum(DigestSize bits);

std::string InitListToString(std::initializer_list<std::string_view> data);

std::string EnumValueToString(DsaType type);
std::string EnumValueToString(DigestSize bits);

bool IsMatchingKeyCurve(EVP_PKEY*, DigestSize bits);

std::unique_ptr<::BIO, decltype(&::BIO_free_all)> MakeBioString(
    std::string_view str);

}  // namespace crypto
