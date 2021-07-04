#include <userver/taxi_config/additional_keys_token.hpp>

namespace taxi_config {

AdditionalKeysToken::AdditionalKeysToken() = default;

AdditionalKeysToken::AdditionalKeysToken(std::shared_ptr<KeyList> keys)
    : keys_(std::move(keys)) {}

}  // namespace taxi_config
