#include <userver/utest/using_namespace_userver.hpp>

/// [TCP sample - component]
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/tcp_acceptor_base.hpp>
#include <userver/utils/daemon_run.hpp>

namespace samples::tcp {

class Hello final : public components::TcpAcceptorBase {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "tcp-hello";

  // Component is valid after construction and is able to accept requests
  using TcpAcceptorBase::TcpAcceptorBase;

  void ProcessSocket(engine::io::Socket&& sock) override {
    std::string data;
    data.resize(2);

    while (true) {
      const auto read_bytes = sock.ReadAll(data.data(), 2, {});
      if (read_bytes != 2 || data != "hi") {
        sock.Close();
        return;
      }

      const auto sent_bytes = sock.SendAll("hello", 5, {});
      if (sent_bytes != 5) {
        return;
      }
    }
  }
};

}  // namespace samples::tcp
/// [TCP sample - component]

/// [TCP sample - main]
int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::MinimalComponentList().Append<samples::tcp::Hello>();

  return utils::DaemonMain(argc, argv, component_list);
}
/// [Hello service sample - main]
