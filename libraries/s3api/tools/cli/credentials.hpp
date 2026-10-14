#pragma once

#include <string>
#include <userver/s3api/models/secret.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {

struct Credentials {
    std::string access_key;
    Secret secret_key;
};

Credentials ParseCredentials(const std::string& contents);
Credentials ReadCredentials(const std::string& path);

}  // namespace s3api::cli

USERVER_NAMESPACE_END
