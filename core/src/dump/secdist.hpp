#pragma once

#include <unordered_map>

#include <userver/dump/operations_encrypted.hpp>
#include <userver/formats/json/value.hpp>

namespace dump {

class Secdist {
 public:
  explicit Secdist(const formats::json::Value& doc);

  SecretKey GetSecretKey(const std::string& cache_name) const;

 private:
  std::unordered_map<std::string, SecretKey> secret_keys_;
};

}  // namespace dump
