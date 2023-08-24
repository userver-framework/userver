#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

namespace samples::hello {

class Hello final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-hello-sample";

  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    return "Hello world!\n";
  }
};

}  // namespace samples::hello

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList().Append<samples::hello::Hello>();
  return utils::DaemonMain(argc, argv, component_list);
}
