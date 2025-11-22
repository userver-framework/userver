#pragma once

#include <memory>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class ClientCore;
}  // namespace clients::http

namespace testsuite {

class HttpAllowedUrlsExtra final {
public:
    void RegisterHttpClient(std::shared_ptr<clients::http::ClientCore> http_client);

    void SetAllowedUrlsExtra(std::vector<std::string>&& urls);

private:
    std::shared_ptr<clients::http::ClientCore> http_client_{nullptr};
};

}  // namespace testsuite

USERVER_NAMESPACE_END
