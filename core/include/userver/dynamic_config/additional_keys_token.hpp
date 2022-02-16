#pragma once

#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {
class AdditionalKeysToken {
 public:
  using KeyList = std::vector<std::string>;

  AdditionalKeysToken();
  explicit AdditionalKeysToken(std::shared_ptr<KeyList> keys);

 private:
  std::shared_ptr<KeyList> keys_;
};

}  // namespace dynamic_config

USERVER_NAMESPACE_END
