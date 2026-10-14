#pragma once

#include <optional>
#include <string>

#include <userver/clients/http/client.hpp>
#include <userver/s3api/clients/s3api.hpp>

#include "credentials.hpp"
#include "options.hpp"

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {

struct Result {
    std::string body;
    std::optional<std::string> etag;
};

ClientPtr MakeClient(clients::http::Client& http_client, const Options& options, const Credentials& credentials);
Result Execute(const Client& client, const Options& options, std::string body);

}  // namespace s3api::cli

USERVER_NAMESPACE_END
