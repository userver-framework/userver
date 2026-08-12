#pragma once

/// @file userver/testsuite/http_allowed_urls_extra.hpp
/// @brief @copybrief testsuite::HttpAllowedUrlsExtra

#include <memory>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class ClientCore;
}  // namespace clients::http

namespace testsuite {

/// @brief Testsuite extra allowed HTTP client URLs
class HttpAllowedUrlsExtra final {
public:
    void RegisterHttpClient(clients::http::ClientCore& http_client);

    void SetAllowedUrlsExtra(std::vector<std::string>&& urls);

private:
    clients::http::ClientCore* http_client_{nullptr};
};

}  // namespace testsuite

USERVER_NAMESPACE_END
