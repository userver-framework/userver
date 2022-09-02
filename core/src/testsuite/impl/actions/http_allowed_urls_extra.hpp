#pragma once

#include <userver/clients/http/client.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/testsuite/http_allowed_urls_extra.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <testsuite/impl/actions/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class HttpAllowedUrlsExtra final : public BaseTestsuiteAction {
 public:
  HttpAllowedUrlsExtra(components::TestsuiteSupport& testsuite_support)
      : http_client_(testsuite_support.GetHttpAllowedUrlsExtra()) {}

  formats::json::Value Perform(
      const formats::json::Value& request_body) const override;

 private:
  testsuite::HttpAllowedUrlsExtra& http_client_;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
