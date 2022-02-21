#pragma once

/// @file userver/dynamic_config/updater/additional_keys_token.hpp
/// @brief @copybrief dynamic_config::AdditionalKeysToken

#include <memory>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

/// @brief Defines the scope where DynamicConfigClientUpdater requests
/// additional configs.
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
