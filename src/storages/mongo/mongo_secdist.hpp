#pragma once

#include <string>
#include <unordered_map>

namespace Json {
class Value;
}  // namespace Json

namespace storages {
namespace mongo {
namespace secdist {

class MongoSettings {
 public:
  explicit MongoSettings(const Json::Value& doc);

  const std::string& GetConnectionString(const std::string& dbalias) const;

 private:
  std::unordered_map<std::string, std::string> settings_;
};

}  // namespace secdist
}  // namespace mongo
}  // namespace storages
