#pragma once

/// @file userver/crypto/basic_types.hpp
/// @brief Common types for crypto module

#include <string>

#include <userver/crypto/exception.hpp>

#ifdef USERVER_FEATURE_WOLFSSL
#include <wolfssl/openssl/evp.h>
#else
/// @cond
struct evp_pkey_st;
struct x509_st;
/// @endcond
#endif

USERVER_NAMESPACE_BEGIN

namespace crypto {

/// @cond
#ifdef USERVER_FEATURE_WOLFSSL
using EVP_PKEY = WOLFSSL_EVP_PKEY;
using X509 = WOLFSSL_X509;
#else
using EVP_PKEY = struct evp_pkey_st;
using X509 = struct x509_st;
#endif
/// @endcond

/// SHA digest size in bits
enum class DigestSize { k160, k256, k384, k512 };

/// Digital signature type
enum class DsaType { kRsa, kEc, kRsaPss };

/// Base class for a crypto algorithm implementation
class NamedAlgo {
 public:
  explicit NamedAlgo(std::string name);
  virtual ~NamedAlgo();

  const std::string& Name() const;

 private:
  const std::string name_;
};

}  // namespace crypto

USERVER_NAMESPACE_END
