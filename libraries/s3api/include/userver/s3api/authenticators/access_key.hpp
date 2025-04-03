#pragma once

/// @file userver/s3api/authenticators/access_key.hpp
/// @brief Authenticator for using `access_key` and `secret_key` for authentication

#include <string>
#include <unordered_map>

#include <userver/s3api/authenticators/interface.hpp>
#include <userver/s3api/models/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::authenticators {

class AccessKey : public Authenticator {
public:
    AccessKey(std::string access_key, Secret secret_key)
        : access_key_{std::move(access_key)}, secret_key_{std::move(secret_key)} {}
    std::unordered_map<std::string, std::string> Auth(const Request& request) const override;
    std::unordered_map<std::string, std::string> Sign(const Request& request, time_t expires) const override;

private:
    std::string access_key_;
    Secret secret_key_;
};

}  // namespace s3api::authenticators

USERVER_NAMESPACE_END
