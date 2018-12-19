#pragma once

#include <string>
#include <unordered_map>

#include <formats/json/value.hpp>

namespace storages {
namespace mongo {
namespace secdist {

class MongoSettings {
 public:
  explicit MongoSettings(const formats::json::Value& doc);

  const std::string& GetConnectionString(const std::string& dbalias) const;

 private:
  std::unordered_map<std::string, std::string> settings_;
};

}  // namespace secdist
}  // namespace mongo
}  // namespace storages
