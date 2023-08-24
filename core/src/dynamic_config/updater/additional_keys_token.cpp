#include <userver/dynamic_config/updater/additional_keys_token.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

AdditionalKeysToken::AdditionalKeysToken() = default;

AdditionalKeysToken::AdditionalKeysToken(std::shared_ptr<KeyList> keys)
    : keys_(std::move(keys)) {}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
