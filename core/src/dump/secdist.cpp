#include <dump/secdist.hpp>

#include <fmt/format.h>

#include <userver/crypto/base64.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

Secdist::Secdist(const formats::json::Value& doc) {
  const auto& section = doc["CACHE_DUMP_SECRET_KEYS"];
  if (section.IsMissing()) return;

  for (const auto& [name, key] : Items(section)) {
    secret_keys_.emplace(name,
                         crypto::base64::Base64Decode(key.As<std::string>()));
  }
}

SecretKey Secdist::GetSecretKey(const std::string& cache_name) const {
  auto it = secret_keys_.find(cache_name);
  if (it != secret_keys_.end()) return it->second;

  throw std::runtime_error(
      fmt::format("Cache dump secret key for cache '{}' not found in secdist "
                  "CACHE_DUMP_SECRET_KEYS",
                  cache_name));
}

}  // namespace dump

USERVER_NAMESPACE_END
