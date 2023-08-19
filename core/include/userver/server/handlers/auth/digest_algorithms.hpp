#pragma once

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

enum class HashAlgTypes { kMD5, kSHA256, kSHA512, kUnknown };

constexpr utils::TrivialBiMap kHashAlgToType = [](auto selector) {
  return selector()
      .Case("md5", HashAlgTypes::kMD5)
      .Case("sha256", HashAlgTypes::kSHA256)
      .Case("sha512", HashAlgTypes::kSHA512);
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
