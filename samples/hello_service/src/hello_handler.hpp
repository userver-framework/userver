#pragma once

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

#include "upy_component.hpp"

namespace samples::hello {

class HelloHandler final : public server::handlers::HttpHandlerBase {
 public:
  HelloHandler(const components::ComponentConfig& config,
               const components::ComponentContext& component_context);

  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "handler-hello-sample";

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  upython::Component& upy_;
};

}  // namespace samples::hello
