#pragma once

#include <unordered_map>

#include <cache/dump/operations_encrypted.hpp>
#include <formats/json/value.hpp>

namespace cache::dump {

class Secdist {
 public:
  explicit Secdist(const formats::json::Value& doc);

  SecretKey GetSecretKey(const std::string& cache_name) const;

 private:
  std::unordered_map<std::string, SecretKey> secret_keys_;
};

}  // namespace cache::dump
