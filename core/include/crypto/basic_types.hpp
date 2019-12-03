#pragma once

#include <string>

#include <crypto/exception.hpp>

/// @cond
struct evp_pkey_st;
/// @endcond

namespace crypto {

/// @cond
using EVP_PKEY = struct evp_pkey_st;
/// @endcond

/// SHA digest size in bits
enum class DigestSize { k256, k384, k512 };

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
