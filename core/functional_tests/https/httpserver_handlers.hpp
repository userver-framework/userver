#pragma once

#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/json_error_builder.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace https {

class HttpServerHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-https-httpserver";

  static inline const std::string kDefaultAnswer = "OK!";

  HttpServerHandler(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : HttpHandlerBase(config, context) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto& type = request.GetArg("type");

    if (type == "cancel") {
      engine::InterruptibleSleepFor(std::chrono::seconds(20));
      if (engine::current_task::IsCancelRequested()) {
        engine::TaskCancellationBlocker block_cancel;
        TESTPOINT("testpoint_cancel", {});
      }
      return kDefaultAnswer;
    }

    UINVARIANT(false, "Unexpected request type");
  }
};

}  // namespace https
