#pragma once

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace tests::handlers {

class Testpoint final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-testpoint";

  using server::handlers::HttpHandlerJsonBase::HttpHandlerJsonBase;

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_body,
      server::request::RequestContext& context) const override;
};

}  // namespace tests::handlers
