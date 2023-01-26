#include <userver/testsuite/http_allowed_urls_extra.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

void HttpAllowedUrlsExtra::RegisterHttpClient(
    clients::http::Client& http_client) {
  http_client_ = &http_client;
}

void HttpAllowedUrlsExtra::SetAllowedUrlsExtra(
    std::vector<std::string>&& urls) {
  if (http_client_) http_client_->SetAllowedUrlsExtra(std::move(urls));
}

}  // namespace testsuite

USERVER_NAMESPACE_END
