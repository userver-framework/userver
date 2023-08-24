#include <testsuite/impl/actions/http_allowed_urls_extra.hpp>

#include <userver/formats/parse/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

formats::json::Value HttpAllowedUrlsExtra::Perform(
    const formats::json::Value& request_body) const {
  auto allowed_urls_extra =
      request_body["allowed_urls_extra"].As<std::vector<std::string>>();

  http_client_.SetAllowedUrlsExtra(std::move(allowed_urls_extra));

  return {};
}

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
