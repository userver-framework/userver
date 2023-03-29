#include <userver/utest/using_namespace_userver.hpp>

/// [TCP sample - component]
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/tcp_acceptor_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace samples::tcp {

class Hello final : public components::TcpAcceptorBase {
 public:
  static constexpr std::string_view kName = "tcp-hello";

  // Component is valid after construction and is able to accept requests
  Hello(const components::ComponentConfig& config,
        const components::ComponentContext& context)
      : TcpAcceptorBase(config, context),
        greeting_(config["greeting"].As<std::string>("hi")) {}

  void ProcessSocket(engine::io::Socket&& sock) override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const std::string greeting_;
};

}  // namespace samples::tcp

/// [TCP sample - component]

namespace samples::tcp {

/// [TCP sample - ProcessSocket]
void Hello::ProcessSocket(engine::io::Socket&& sock) {
  std::string data;
  data.resize(2);

  while (!engine::current_task::ShouldCancel()) {
    const auto read_bytes = sock.ReadAll(data.data(), 2, {});
    if (read_bytes != 2 || data != "hi") {
      sock.Close();
      return;
    }

    const auto sent_bytes =
        sock.SendAll(greeting_.data(), greeting_.size(), {});
    if (sent_bytes != greeting_.size()) {
      return;
    }
  }
}
/// [TCP sample - ProcessSocket]

/// [TCP sample - GetStaticConfigSchema]
yaml_config::Schema Hello::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<TcpAcceptorBase>(R"(
    type: object
    description: |
      Component for accepting incoming TCP connections and responding with some
      greeting as long as the client sends 'hi'.
    additionalProperties: false
    properties:
      greeting:
          type: string
          description: greeting to send to client
          defaultDescription: hi
  )");
}
/// [TCP sample - GetStaticConfigSchema]

}  // namespace samples::tcp

/// [TCP sample - main]
int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::MinimalComponentList().Append<samples::tcp::Hello>();

  return utils::DaemonMain(argc, argv, component_list);
}
/// [TCP sample - main]
