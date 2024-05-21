#include <userver/utest/using_namespace_userver.hpp>

/// [Hello service sample - component]
#include "hello_service.hpp"

#include <userver/server/handlers/http_handler_base.hpp>

#include "say_hello.hpp"

namespace samples::hello {

namespace {

class Hello final : public server::handlers::HttpHandlerBase {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "handler-hello-sample";

  // Component is valid after construction and is able to accept requests
  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    return samples::hello::SayHelloTo(request.GetArg("name"));
  }
};

}  // namespace

void AppendHello(components::ComponentList& component_list) {
  component_list.Append<Hello>();
}

}  // namespace samples::hello
/// [Hello service sample - component]
