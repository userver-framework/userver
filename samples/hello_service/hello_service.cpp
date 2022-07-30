#include <userver/utest/using_namespace_userver.hpp>

/// [Hello service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/periodic_task.hpp>

#include <thread>

namespace samples::hello {

class Hello final : public server::handlers::HttpHandlerBase {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "handler-hello-sample";

  Hello(const components::ComponentConfig& config,
        const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase{config, context},
        background_{engine::AsyncNoSpan([] {
          while (!engine::current_task::ShouldCancel()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
            engine::Yield();
          }
        })} {

  }

  ~Hello() override {
    background_.SyncCancel();
  }

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    return "Hello world!\n";
  }

 private:
  engine::TaskWithResult<void> background_;
};

}  // namespace samples::hello
/// [Hello service sample - component]

/// [Hello service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList().Append<samples::hello::Hello>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Hello service sample - main]
