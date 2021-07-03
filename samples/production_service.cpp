/// [Production service sample - main]
#include <components/common_component_list.hpp>
#include <components/common_server_component_list.hpp>
#include <utils/daemon_run.hpp>

#include <server/handlers/ping.hpp>
#include <storages/secdist/component.hpp>

int main(int argc, char* argv[]) {
  const auto component_list =
      ::components::ComponentList()
          .AppendComponentList(components::CommonComponentList())
          .AppendComponentList(components::CommonServerComponentList())
          .Append<components::Secdist>()
          .Append<server::handlers::Ping>()

      // Put your handlers and componenets here
      ;

  return utils::DaemonMain(argc, argv, component_list);
}
/// [Production service sample - main]
