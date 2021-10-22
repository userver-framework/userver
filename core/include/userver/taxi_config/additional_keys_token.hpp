#pragma once

#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

class AdditionalKeysToken {
 public:
  using KeyList = std::vector<std::string>;

  AdditionalKeysToken();
  explicit AdditionalKeysToken(std::shared_ptr<KeyList> keys);

 private:
  std::shared_ptr<KeyList> keys_;
};

}  // namespace taxi_config

USERVER_NAMESPACE_END
