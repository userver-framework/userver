#include <cache/dump/secdist.hpp>

namespace cache::dump {

Secdist::Secdist(const formats::json::Value& doc) {
  const auto& section = doc["CACHE_DUMP_SECRET_KEYS"];
  secret_keys_ = section.As<std::unordered_map<std::string, SecretKey>>({});
}

SecretKey Secdist::GetSecretKey(const std::string& cache_name) const {
  auto it = secret_keys_.find(cache_name);
  if (it != secret_keys_.end()) return it->second;

  throw std::runtime_error(
      fmt::format("Cache dump secret key for cache '{}' not found in secdist "
                  "CACHE_DUMP_SECRET_KEYS",
                  cache_name));
}

}  // namespace cache::dump
