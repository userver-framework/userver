#pragma once

#include <unordered_map>

#include <userver/dump/operations_encrypted.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

class Secdist {
 public:
  explicit Secdist(const formats::json::Value& doc);

  SecretKey GetSecretKey(const std::string& cache_name) const;

 private:
  std::unordered_map<std::string, SecretKey> secret_keys_;
};

}  // namespace dump

USERVER_NAMESPACE_END
