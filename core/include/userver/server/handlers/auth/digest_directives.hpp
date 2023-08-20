#pragma once

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

/// @brief Supported hashing algorithms
enum class HashAlgTypes {
  kMD5,     ///< MD5 algorithm
  kSHA256,  ///< SHA256 algorithm
  kSHA512,  ///< SHA512 algortihm
  kUnknown  ///< Unknown algorithm
};

/// @brief Supported qop-options
enum class QopTypes {
  kAuth,     ///< `The value "auth" indicates authentication` from
             ///< https://datatracker.ietf.org/doc/html/rfc2617#section-3.2.1
  kUnknown,  ///< Unknown qop-value
};

constexpr utils::TrivialBiMap kHashAlgToType = [](auto selector) {
  return selector()
      .Case("md5", HashAlgTypes::kMD5)
      .Case("sha256", HashAlgTypes::kSHA256)
      .Case("sha512", HashAlgTypes::kSHA512);
};

constexpr utils::TrivialBiMap kQopToType = [](auto selector) {
  return selector().Case("auth", QopTypes::kAuth);
};

// enum

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
